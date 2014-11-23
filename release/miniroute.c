#include "miniroute.h"


// Miniroute data structures
cache_table_t cache;
semaphore_t cache_mutex = NULL;
unsigned int id = 0; // id for Route Discovery and Route Reply packets // How many to allow? Unsigned?


/* Performs any initialization of the miniroute layer, if required. */
void miniroute_initialize() {
	// Initialize mutex for cache accesses
	cache_mutex = semaphore_create();
    semaphore_initialize(cache_mutex, 1);

    // Init Cache
    cache = cache_table_new();
}

/* sends a miniroute packet, automatically discovering the path if necessary. See description in the
 * .h file.
 */
int miniroute_send_pkt(network_address_t dest_address, int hdr_len, char* hdr, int data_len, char* data) {
	network_address_t next_hop;
	routing_header_t routing_hdr; // Routing layer header
	//char* hdr_full[sizeof(struct routing_header) + hdr_len]; // routing_hdr followed by hdr
	int path_len, pathfound, i, j;
	// int hdr_full_len;
	cache_elem_t dest_elem;
	char payload[hdr_len + data_len];

	// Allocate new header for packet
	routing_hdr = malloc(sizeof(struct routing_header));
	if (routing_hdr == NULL) {	// Could not allocate header
		fprintf(stderr, "ERROR: miniroute_send_pkt() failed to malloc expanded header\n");
		return -1;
	}

	semaphore_P(cache_mutex);
	dest_elem = cache_table_get(cache, dest_address);
	semaphore_V(cache_mutex);


	if (dest_elem == NULL) { // Need to Discover path
		pathfound = miniroute_discover_path(dest_address); // Run path discovery algorithm -> 
		if (pathfound <= 0) {
			fprintf(stderr, "Unable to locate path to specified destination\n");
			return -1;
		}

		semaphore_P(cache_mutex);
		dest_elem = cache_table_get(cache, dest_address);
		semaphore_V(cache_mutex);
	} else if (dest_elem->path == NULL) {
		fprintf(stderr, "ERROR: miniroute_send_pkt() found non-null cache entry with null path\n");
		return -1;
	}

	// Build routing_hdr with updated fields
	routing_hdr->routing_packet_type = ROUTING_DATA;
	pack_address(routing_hdr->destination, dest_address);
	pack_unsigned_int(routing_hdr->ttl, MAX_ROUTE_LENGTH);
	pack_unsigned_int(routing_hdr->path_len, path_len);
	/*for (i = 0; i < MAX_ROUTE_LENGTH; i++) {
		pack_address(routing_hdr->path[i], path[i]);
	}*/
	for (i = 0; i < MAX_ROUTE_LENGTH; i++) {
		for (j = 0; j < 8; j++) {
			routing_hdr->path[i][j] = dest_elem->path[i][j];
		}
	}

	// Build payload
	for (i = 0; i < hdr_len + data_len; i++) {
		if (i < hdr_len) {
			payload[i] = hdr[i];
		} else { // Copy data
			payload[i] = data[i - hdr_len];
		}
	}

	// Assign first address in path as the next hop destination
	unpack_address(&routing_hdr->path[1][0], next_hop);

	return network_send_pkt(next_hop, sizeof(struct routing_header), (char*) routing_hdr, data_len + hdr_len, payload);
}


/* Hashes a network_address_t into a 16 bit unsigned int */
unsigned short hash_address(network_address_t address) {
	unsigned int result = 0;
	int counter;

	for (counter = 0; counter < 3; counter++)
		result ^= ((unsigned short*) address)[counter];

	return result % 65521;
}



/* ROUTE DISCOVERY METHODS */

/* 
 * Allows the thread to be woken if either a packet arrives or the timeout finishes.
 * Requires functions that access socket queues ....
 *
 * Precondition: sema is initialized as a counting semaphore (0)
 * 
 * Returns 0 if discovery packet was received, 1 if woke up without discovery, -1 on error
 */
int wait_for_discovery_or_timeout(semaphore_t sema, alarm_t* alarm, int timeout) {
	if (sema == NULL) {
		fprintf(stderr, "ERROR: wait_for_discovery_or_timeout() passed uninitialized semaphore\n");
		return -1;
	}

	*alarm = (alarm_t) register_alarm(timeout, (alarm_handler_t) semaphore_V, (void*) sema);
	
	semaphore_P(sema);

	if (*alarm == NULL) {
		return 0;
	} else {
		if ((*alarm)->executed) {
			if (deregister_alarm(*alarm)) {
				*alarm = NULL;
				return 1;
			} else {
				fprintf(stderr, "ERROR: wait_for_discovery_or_timeout() woke up with conflicting executed statuses\n");
				return -1;
			}
		} else {
			fprintf(stderr, "ERROR: wait_for_discovery_or_timeout() woke up with not executed non-null alarm\n");
			return -1;
		}
	}
}


/* Route discovery algorithm that calls network_bcast_pkt() to find the path to dest_address and stores this path in the path argument.
 * Similar to minisocket.c implementation of packet retransmission.
 * Returns 1 if a valid path is found, 0 if no path is found, or -1 on error.
 */
int miniroute_discover_path(network_address_t dest_address) {
	int send_attempts, timeout, received_next_packet;
	network_address_t myaddr;
	cache_elem_t dest_elem = NULL;
	semaphore_t dest_sema = NULL;
	alarm_t dest_alarm = NULL; // Issue here is that we have no local data structure that encapsulates a single alarm -> so we can't in net_handler just
							// decide to deregister something, UNLESS we make it visible there. Instead, we can have the first thing we do after waking up from
							// discovery_blocking be to check whether alarm exec'd or not 
	int status = 0;
	int found = 0;

	send_attempts = 0;
	timeout = DISCOVERY_TIMEOUT;
	received_next_packet = 0;
	network_get_my_address(myaddr);
	
	semaphore_P(cache_mutex);
	// Quick lookup in cache
	dest_elem = cache_table_get(cache, dest_address);
	if (dest_elem == NULL){
		//create element
		dest_elem = cache_table_insert(cache, dest_address, dest_sema, dest_alarm, NULL);	//don't specify path here, leave NULL
		if (dest_elem == NULL) {
			fprintf(stderr, "ERROR: miniroute_discover_path() failed to insert new cache element\n");
			semaphore_V(cache_mutex);
			return -1;
		}
	}
	semaphore_V(cache_mutex);

	
	semaphore_P(dest_elem->mutex);

	if (dest_elem->path == NULL){ // No path here. Checked because another thread might have run ahead of me and populated this

		// Allocate new header for packet
		routing_hdr = malloc(sizeof(struct routing_header));
		if (routing_hdr == NULL) {	// Could not allocate header
			fprintf(stderr, "ERROR: miniroute_discover_path() failed to malloc expanded header\n");
			return -1;
		}

		// Build routing_hdr with updated fields
		routing_hdr->routing_packet_type = ROUTING_ROUTE_DISCOVERY;
		pack_address(routing_hdr->destination, dest_address);
		pack_unsigned_int(routing_hdr->id, SOMETHING); //                  FILL IN HERE
		pack_unsigned_int(routing_hdr->ttl, MAX_ROUTE_LENGTH);
		pack_unsigned_int(routing_hdr->path_len, 1);
		pack_address(routing_hdr->path[0], myaddr);

		while (send_attempts < MAX_SEND_ATTEMPTS && !received_next_packet) {
			if (network_bcast_pkt(sizeof(struct routing_header), routing_hdr, 0, NULL) < 0) {
				fprintf(stderr, "ERROR: miniroute_discover_path() failed to successfully execute network_bcast_pkt()\n");
				semaphore_V(dest_elem->mutex);
				return -1; // Failure
			}

			// Block here until timeout expires (and alarm is thus deregistered) or packet is received, deregistering the pending alarm		
			status = wait_for_discovery_or_timeout(dest_elem->timeout, &(dest_elem->reply), timeout);

			if (status == -1) { // Error
				return -1;
			} else if (status == 1) { // Timed out
				send_attempts++;
			} else if (status == 0) { // Received reply packet
				received_next_packet = 1;
			}
		}
	} else {
		received_next_packet = 1; // path already defined
	}
		
	semaphore_V(dest_elem->mutex);
	
	return received_next_packet;
}
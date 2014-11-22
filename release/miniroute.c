#include "miniroute.h"


// Miniroute data structures
// Cache (hashtable)
semaphore_t cache_mutex = NULL;
int id = 0; // id for Route Discovery and Route Reply packets // How many to allow? Unsigned?




/* Performs any initialization of the miniroute layer, if required. */
void miniroute_initialize() {
	// Initialize mutex for cache accesses
	cache_mutex = semaphore_create();
    semaphore_initialize(cache_mutex, 1);

    // Init Cache

    // Init queue of routing reply packets?
}

/* sends a miniroute packet, automatically discovering the path if necessary. See description in the
 * .h file.
 */
int miniroute_send_pkt(network_address_t dest_address, int hdr_len, char* hdr, int data_len, char* data) {
	network_address_t next_hop;
	routing_header_t routing_hdr; // Routing layer header
	char* hdr_full[sizeof(struct routing_header) + hdr_len]; // routing_hdr followed by hdr
	char path[MAX_ROUTE_LENGTH][8]; // Path to dest_address
	int hdr_full_len, i;

	hdr_full_len = sizeof(struct routing_header) + hdr_len;

	// Allocate new header for packet
	routing_hdr = malloc(sizeof(struct routing_header));
	if (routing_hdr == NULL) {	// Could not allocate header
		fprintf(stderr, "ERROR: miniroute_send_pkt() failed to malloc expanded header\n");
		return -1;
	}

	semaphore_P(cache_mutex);
	// Consult cache (assume some other thread cleans up old entries in the cache)
	// get_cache(dest_address)

	// If not in cache, create cache element so you can block on that particular thing

	// Have result -> either in cache (assume valid if so), or not
	semaphore_V(cache_mutex);


	if (dest_address not in cache) { // Need to Discover path

		pathfound = miniroute_discover_path(dest_address, path); // Run path discovery algorithm -> 
		if (pathfound <= 0) {
			fprintf(stderr, "Unable to locate path to specified destination\n");
			return -1;
		}

	}	
	// path = pathfound//(elem lookup from hashtable)->path;



	// Build routing_hdr with updated fields
	routing_hdr->routing_packet_type = ROUTING_DATA;
	pack_address(routing_hdr->destination, dest_address);
	pack_unsigned_int(routing_hdr->id, SOMETHING);
	pack_unsigned_int(routing_hdr->ttl, SOMETHING);
	pack_unsigned_int(routing_hdr->path_len, SOMETHING);
	for (i = 0; i < MAX_ROUTE_LENGTH; i++) {
		pack_address(routing_hdr->path[i], path[i]);
	}

	// Build hdr_full by appending hdr after routing_hdr
	for (i = 0; i < hdr_full_len; i++) {
		if (i < sizeof(struct routing_header)) { // Copy from routing layer header
			hdr_full[i] = ((char*) routing_hdr)[i];
		} else { // Copy from transport layer header
			hdr_full[i] = hdr[i - sizeof(struct routing_header)];
		}
	}

	unpack_address(path[0], next_hop); // Assign first address in path as the next hop destination

	return network_send_pkt(next_hop, hdr_full_len, hdr_full, data_len, data);
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
	if (alarm->executed /* *alarm != NULL*/) {
		return alarmstatus;
		fprintf(stderr, "Didn't discover packet\n");
	} else {
		fprintf(stderr, "Discovered packet!\n");
		return 0;
	}
}


	// if discovery needs to be initiated, the thread calling miniroute_send_pkt() should be blocked until the new route is discovered 
	// or the discovery is retried three times and times out in all three cases.

 	// -> broadcast and block
		// When are you unblocked? When (in nethandler) you receive a route discovery packet AND update path to state

/* Route discovery algorithm that calls network_bcast_pkt() to find the path to dest_address and stores this path in the path argument.
 * Similar to minisocket.c implementation of packet retransmission.
 * Returns total path length if a valid path is found, 0 if no path is found, or -1 on error.
 */
int miniroute_discover_path(network_address_t dest_address  /*, char* path*/) {
	int send_attempts, timeout, received_next_packet;
	char* path = NULL;
	cache_elem_t dest_elem = NULL;
	semaphore_t dest_sema = NULL;
	alarm_t dest_alarm = NULL; // Issue here is that we have no local data structure that encapsulates a single alarm -> so we can't in net_handler just
							// decide to deregister something, UNLESS we make it visible there. Instead, we can have the first thing we do after waking up from
							// discovery_blocking be to check whether alarm exec'd or not 
	int exec = 0;
	int found = 0;


	send_attempts = 0;
	timeout = DISCOVERY_TIMEOUT;
	received_next_packet = 0;

	
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
		pack_unsigned_int(routing_hdr->ttl, SOMETHING); //                  FILL IN HERE
		pack_unsigned_int(routing_hdr->path_len, SOMETHING); //                  FILL IN HERE
		for (i = 0; i < MAX_ROUTE_LENGTH; i++) {
			pack_address(routing_hdr->path[i], path[i]);
		}

		while (send_attempts < MAX_SEND_ATTEMPTS && !received_next_packet) {
			if (network_bcast_pkt(sizeof(struct routing_header), routing_hdr, 0, NULL) < 0) {
				fprintf(stderr, "ERROR: miniroute_discover_path() failed to successfully execute network_bcast_pkt()\n");
				semaphore_V(dest_elem->mutex);
				return -1; // Failure
			}

			// Block here until timeout expires (and alarm is thus deregistered) or packet is received, deregistering the pending alarm		
			exec = wait_for_discovery_or_timeout(dest_elem->timeout, dest_elem->reply, timeout);
			// fprintf(stderr, "executed: %i\n", exec);

			if (exec) {
				send_attempts++;
			} else { // Valid discovery reply packet received
				received_next_packet = 1;
				dest_elem->reply = NULL;		// No active retransmission alarm
			}
		}

	}
		
	semaphore_V(dest_elem->mutex);
	
	return received_next_packet;

}



// /* Used when we want to retransmit a given packet a certain number of times while a desired response has not been received 
// 	(relies on network_handler to get said response). Return -1 on Failure, 0 if Timed out, 1 if Received packet. */
// int retransmit_packet(semaphore_t sema, char* hdr, int data_len, char* data, minisocket_error *error) {
// 	int send_attempts, timeout, received_next_packet;
// 	int exec = 0;

// 	send_attempts = 0;
// 	timeout = DISCOVERY_TIMEOUT;
// 	received_next_packet = 0;

// 	// // "Reset" alarm field
// 	// socket->alarm = NULL;

// 	while (send_attempts < MAX_DISC_ATTEMPTS && !received_next_packet) {
// 		if (miniroute_send_pkt(socket->dest_address, sizeof(struct mini_header_reliable), hdr, data_len, data) < 0) {
// 			fprintf(stderr, "ERROR: retransmit_packet() failed to successfully execute network_send_pkt()\n");
// 			// *error = SOCKET_SENDERROR;
// 			return -1; // Failure
// 		}
// 		fprintf(stderr, "DEBUG: Sent %i with (seq = %i, ack = %i) attempt %i\n", ((mini_header_reliable_t) hdr)->message_type, unpack_unsigned_int(((mini_header_reliable_t) hdr)->seq_number), unpack_unsigned_int(((mini_header_reliable_t) hdr)->ack_number), send_attempts + 1);

// 		// Block here until timeout expires (and alarm is thus deregistered) or packet is received, deregistering the pending alarm		
// 		// CHECK: need to enforce mutual exclusion here?
// 		exec = wait_for_discovery_or_timeout(socket->datagrams_ready, &(socket->alarm), timeout);	//CHECK: Is this the CORRECT semaphore to block on?
// 		fprintf(stderr, "executed: %i\n", exec);

// 		if (exec) {
// 			send_attempts++;
// 		} else { // Valid discovery reply packet received
// 			received_next_packet = 1;

// 			// socket->alarm = NULL;		// No active retransmission alarm
// 		}
// 	}

// 	return received_next_packet;
// }

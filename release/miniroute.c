#include "miniroute.h"

/* Performs any initialization of the miniroute layer, if required. */
void miniroute_initialize() {

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
	hdr_full = malloc(sizeof(struct routing_header));
	if (hdr == NULL) {	// Could not allocate header
		fprintf(stderr, "ERROR: miniroute_send_pkt() failed to malloc expanded header\n");
		return -1;
	}

	if (dest_address not in cache) { // Discover path if 
		pathfound = miniroute_discover_path(dest_address, path); // Run path discovery algorithm
		if (pathfound <= 0) {
			fprintf(stderr, "Unable to locate path to specified destination\n");
			return -1;
		}
	}

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

/* hashes a network_address_t into a 16 bit unsigned int */
unsigned short hash_address(network_address_t address) {
	unsigned int result = 0;
	int counter;

	for (counter = 0; counter < 3; counter++)
		result ^= ((unsigned short*) address)[counter];

	return result % 65521;
}
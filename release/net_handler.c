#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include "minithread.h"

#include "miniheader.h"
#include "minimsg.h"
#include "minisocket.h"

void network_handler(network_interrupt_arg_t* pkt) {
	network_address_t my_addr, destination;
	int id, ttl, path_len
	char* buffer;
	char* path;
	char routing_packet_type;

	interrupt_level_t old_level = set_interrupt_level(DISABLED); // Disable interrupts

	network_get_my_address(my_addr); // Set my address

	// Extract packet routing_header contents
	buffer = pkt->buffer;
	routing_packet_type = buffer[0];
	unpack_address(&buffer[1], destination);
	id = unpack_unsigned_int(&buffer[9]);
	ttl = unpack_unsigned_int(&buffer[13]);
	path_len = unpack_unsigned_int(&buffer[17]);
	path = &buffer[21];

	if (routing_packet_type == ROUTING_DATA) {
		// DATA


	} else if (routing_packet_type == ROUTING_ROUTE_DISCOVERY) {
		// ROUTE DISCOVERY

		// If I AM the destination
			// Send appropriate route reply (after reversing path), same ID, setting destination

		// Else 
			// I should add my address to this packet's path, increment pathlen
			// decrease ttl by 1
				//If ttl not 0
					// If I havent seen this same discovery guy recently (how to define recently?), AKA same id and from same src machine (first thing in path field)
						// broadcast it onwards


	} else if (routing_packet_type == ROUTING_ROUTE_REPLY) {
		// ROUTE REPLY

		// If I AM the dest of reply
			// Check the cache for existence of the element (should be there)
			// Check path is set, if so do nothing, else update:
				// Reverse the result to get desired path
				// Put result in some queue for thread asking for route discovery to consume
				// Unblock that thread by Ving on some semaphore

		// decrease ttl by 1
			// If ttl not 0
				// If I havent seen this same reply guy recently (how to define recently?), AKA same id and from same src machine (first thing in path field)
					// (uni)cast it onwards, through the intermediate nodes

	} else {
		fprintf(stderr, "ERROR: network_handler() received packet with invalid routing_packet_type\n");
	}

	// Restore old interrupt level
	set_interrupt_level(old_level);
}
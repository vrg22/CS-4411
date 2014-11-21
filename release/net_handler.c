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
	} else if (routing_packet_type == ROUTING_ROUTE_REPLY) {
		// ROUTE REPLY
	} else {
		fprintf(stderr, "ERROR: network_handler() received packet with invalid routing_packet_type\n");
	}

	// Restore old interrupt level
	set_interrupt_level(old_level);
}
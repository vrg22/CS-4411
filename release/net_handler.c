#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include "minithread.h"

#include "miniheader.h"
#include "minimsg.h"
#include "minisocket.h"

void network_handler(network_interrupt_arg_t* pkt) {
	network_address_t my_addr, destination, reply_dest, next_hop, temp;
	cache_elem_t dest_elem;
	int id, ttl, path_len;
	int i, j;
	char* buffer;
	char path[MAX_ROUTE_LENGTH][8];
	char new_path[MAX_ROUTE_LENGTH][8];
	char routing_packet_type;

	char protocol, msg_type;
	network_address_t src_addr;
	unsigned short dest_port;
	unsigned int seq_num;
	unsigned int ack_num;	// of the packet
	char* subbuffer;
	mini_header_reliable_t hdr;

	interrupt_level_t old_level = set_interrupt_level(DISABLED); // Disable interrupts

	network_get_my_address(my_addr); // Set my address

	// Extract packet routing_header contents
	buffer = pkt->buffer;
	routing_packet_type = buffer[0];
	unpack_address(&buffer[1], destination);
	id = unpack_unsigned_int(&buffer[9]);
	ttl = unpack_unsigned_int(&buffer[13]);
	path_len = unpack_unsigned_int(&buffer[17]); // Path length of received packet
	for (i = 0; i < path_len; i++) {	// Extract path from packet
		for (j = 0; j < 8; j++) {
			path[i][j] = buffer[21 + 8*i + j];
		}
	}

	if (routing_packet_type == ROUTING_DATA) {
		if (network_compare_network_addresses(destination, myaddr)) {
			// Basic packet parameters
			subbuffer = &buffer[sizeof(struct routing_header)];
			// length = pkt->size;   //Header size + Data size

			// Extract req'd info from packet header
			protocol = subbuffer[0];


			//Handle as a UDP datagram
			if (protocol == PROTOCOL_MINIDATAGRAM) {
				unpack_address(&subbuffer[11], dest_addr); //Ultimate packet destination: may need to send it away if it doesn't match my address
				dest_port = unpack_unsigned_short(&subbuffer[19]);

				if (network_compare_network_addresses(dest_addr, my_addr) != 0) {  // This packet is meant for me
					if (ports[dest_port] != NULL) {     //Locally unbound port exists
						if (ports[dest_port]->u.unbound.incoming_data != NULL) {  //Queue at locally unbound port has been initialized
							//Put PTR TO ENTIRE PACKET (type: network_interrupt_arg_t*) in the queue at that port
							queue_append(ports[dest_port]->u.unbound.incoming_data, /*(void*)*/ pkt);   //(minimsg_t) subbuffer, data;
							semaphore_V(ports[dest_port]->u.unbound.datagrams_ready);   // V on semaphore
						} else
							fprintf(stderr, "Network handler: queue not set. ERROR\n");
					} else
						fprintf(stderr, "Network handler: dest port doesn't exist. Dropping packet\n");
				} else
					fprintf(stderr, "Network handler: address not for me. Dropping packet\n");
			} 

			//Handle as TCP-reliable datagram (protocol == PROTOCOL_MINISTREAM)
			else {
				//Unpack relevant fields
				unpack_address(&subbuffer[1], src_addr); // Packet's original source address
				// src_port = unpack_unsigned_short(&subbuffer[9]); // Packet's original source port

				msg_type = subbuffer[21];
				seq_num = unpack_unsigned_int(&subbuffer[22]);
				ack_num = unpack_unsigned_int(&subbuffer[26]);

				unpack_address(&subbuffer[11], dest_addr); // Ultimate packet destination
				dest_port = unpack_unsigned_short(&subbuffer[19]); // Ultimate packet destination's port

				fprintf(stderr, "Got somethin in net handler (%i, %i)\n", seq_num, ack_num);

				/*Handle Packet*/
				if (network_compare_network_addresses(dest_addr, my_addr)) {  // This packet IS meant for me
					if (sockets[dest_port] != NULL) { // Local socket exists
						fprintf(stderr, "Local socket exists (%i, %i)\n", sockets[dest_port]->seqnum, sockets[dest_port]->acknum);
						// Received packet has valid ACK and SEQ #s wrt my local ACK and SEQ #s
						if ((ack_num == sockets[dest_port]->seqnum) && (seq_num <= sockets[dest_port]->acknum + 1)) {
							// fprintf(stderr, "Here's this 'lil fucker %i\n", msg_type == MSG_ACK);
							// Take actions depending on packet type
							if (msg_type == MSG_ACK) {
								fprintf(stderr, "Got an ACK of some sort\n");
								// Consider cases of empty ACK vs. data ACK
								if (pkt->size == sizeof(struct mini_header_reliable)) { // Empty ACK
									if (sockets[dest_port]->alarm != NULL && !sockets[dest_port]->alarm->executed) {
										// semaphore_V(sockets[dest_port]->timeout);
										semaphore_V(sockets[dest_port]->datagrams_ready);
										fprintf(stderr, "Got empty ACK packet\n");
									}
									deregister_alarm(sockets[dest_port]->alarm);
									sockets[dest_port]->alarm = NULL;
									// sockets[dest_port]->alarm->executed = 1;
								} else { // Data ACK
									fprintf(stderr, "Got data ACK packet\n");
									if (seq_num == sockets[dest_port]->acknum + 1) { // First arrival of message
										sockets[dest_port]->acknum++;

										// Treat data ACK as an empty ACK if something is awaiting an ACK
										if (sockets[dest_port]->alarm != NULL && !sockets[dest_port]->alarm->executed) {
											// semaphore_V(sockets[dest_port]->timeout);
											semaphore_V(sockets[dest_port]->datagrams_ready);
										}
										deregister_alarm(sockets[dest_port]->alarm);
										sockets[dest_port]->alarm = NULL;
										// sockets[dest_port]->alarm->executed = 1;

										fprintf(stderr, "Got data ACK packet\n");

										queue_append(sockets[dest_port]->incoming_data, pkt);
										semaphore_V(sockets[dest_port]->datagrams_ready);
									}

									// Send an empty ACK back
									hdr = malloc(sizeof(struct mini_header_reliable));	// Allocate new header for ACK packet
									if (hdr == NULL) {	// Could not allocate header
										fprintf(stderr, "ERROR: network_handler() failed to malloc new mini_header_reliable\n");
										return;
									}
									set_header(sockets[dest_port], hdr, MSG_ACK);
									if (network_send_pkt(sockets[dest_port]->dest_address, sizeof(struct mini_header_reliable), (char*) hdr, 0, NULL) < 0) {
										fprintf(stderr, "ERROR: network_handler() failed to use network_send_pkt()\n");
										return;
									}
									fprintf(stderr, "Sent empty ACK packet\n");
								}
							} else if (msg_type == MSG_SYN) {
								fprintf(stderr, "Got MSG_SYN packet\n");
								if (!sockets[dest_port]->active) { // Socket not previously in communication
									sockets[dest_port]->acknum++;
									sockets[dest_port]->active = 1;
									unpack_address(&subbuffer[1], sockets[dest_port]->dest_address);
									//network_address_copy(src_addr, sockets[dest_port]->dest_address);
									sockets[dest_port]->remote_port = unpack_unsigned_short(&subbuffer[9]);
									semaphore_V(sockets[dest_port]->wait_syn);
								} else { // Socket in use - send FIN
									hdr = malloc(sizeof(struct mini_header_reliable));	// Allocate new header for FIN packet
									if (hdr == NULL) {	// Could not allocate header
										fprintf(stderr, "ERROR: network_handler() failed to malloc new mini_header_reliable\n");
										return;
									}
									sockets[dest_port]->seqnum++;
									set_header(sockets[dest_port], hdr, MSG_FIN);
									if (network_send_pkt(sockets[dest_port]->dest_address, sizeof(struct mini_header_reliable), (char*) hdr, 0, NULL) < 0) {
										fprintf(stderr, "ERROR: network_handler() failed to use network_send_pkt()\n");
										return;
									}
								}
							} else if (msg_type == MSG_SYNACK) {
								fprintf(stderr, "Got MSG_SYNACK packet\n");
								// Disable timeout alert
								if (sockets[dest_port]->alarm != NULL && !sockets[dest_port]->alarm->executed) {
									sockets[dest_port]->acknum++;
									// semaphore_V(sockets[dest_port]->timeout);
									semaphore_V(sockets[dest_port]->datagrams_ready);
								}
								deregister_alarm(sockets[dest_port]->alarm);
								sockets[dest_port]->alarm = NULL;
								// sockets[dest_port]->alarm->executed = 1;

								// Send an empty ACK back
								hdr = malloc(sizeof(struct mini_header_reliable));	// Allocate new header for ACK packet
								if (hdr == NULL) {	// Could not allocate header
									fprintf(stderr, "ERROR: network_handler() failed to malloc new mini_header_reliable\n");
									return;
								}
								set_header(sockets[dest_port], hdr, MSG_ACK);
								if (network_send_pkt(sockets[dest_port]->dest_address, sizeof(struct mini_header_reliable), (char*) hdr, 0, NULL) < 0) {
									fprintf(stderr, "ERROR: network_handler() failed to use network_send_pkt()\n");
									return;
								}
								fprintf(stderr, "Sent empty ACK packet (%i, %i)\n", sockets[dest_port]->seqnum, sockets[dest_port]->acknum);
							} else if (msg_type == MSG_FIN) {
								// THROW FATASS ERRORS
							}
						}
					} else
						fprintf(stderr, "Network handler: local socket at dest port doesn't exist. Dropping packet\n");
				} else
				   fprintf(stderr, "Network handler: address not for me. Dropping packet\n");
			}
		} else {
			ttl--;
			if (ttl > 0 && MAX_ROUTE_LENGTH - ttl + 1 < path_len) {
				unpack_address(path[MAX_ROUTE_LENGTH - ttl + 1], next_hop);
			}

			// Build header
			network_send_pkt();
		}
	} else if (routing_packet_type == ROUTING_ROUTE_DISCOVERY) {
		if (network_compare_network_addresses(destination, myaddr)) { // I am final destination
			unpack_address(&path[0][0], reply_dest); // Set final destination for reply packet
			
			// Add ourselves to end of path
			pack_address(&path[path_len][0], myaddr);
			path_len++;

			// Reverse path
			for (i = path_len; i > 0; i--) {
				for (j = 0; j < 8; j++) {
					new_path[path_len - i][j] = path[i-1][j];
				}
			}

			unpack_address(&path[1][0], next_hop); // Set destination address for reply packet's next hop
			
			// Build header
			network_send_pkt();
		} else { // Discovery packet needs to be rebroadcasted
			ttl--;
			if (ttl > 0) {
				// Check that I have not gotten this before
				for (i = 0; i < path_len; i++) {
					unpack_address(&path[i][0], temp);
					if (network_compare_network_addresses(temp, myaddr)) {
						// fprintf(stderr, "ERROR: network_handler() got a discovery pkt that reached me before\n");
						return;
					}
				}

				// Add ourselves to end of path
				pack_address(&path[path_len][0], myaddr);
				path_len++;
				
				// Rebroadcast packet w/ updated params
			}
		}
	} else if (routing_packet_type == ROUTING_ROUTE_REPLY) {
		if (network_compare_network_addresses(destination, myaddr)) { // I am final destination
			dest_elem = cache_table_get(cache, destination);
			if (dest_elem == NULL) {
				fprintf(stderr, "ERROR: network_handler() found a reply for null cache destination\n");
				return;
			}
			if (dest_elem->path == NULL) { // Only update cache entry's path if not already completed
				if (dest_elem->reply != NULL && dest_elem->reply->executed == 0) { // Timout has not yet occured and reply has not already been received
					deregister_alarm(dest_elem->reply);
					dest_elem->reply == NULL;

					// Reverse path
					for (i = path_len; i >= 0; i--) {
						for (j = 0; j < 8; j++) {
							dest_elem->path[path_len - i][j] = path[i][j];
						}
					}
					dest_elem->path_len = path_len; // Update path length

					semaphore_V(dest_elem->timeout);
				}
			} else {
				fprintf(stderr, "ERROR: network_handler() found a reply for non-null cache path\n");
				return;
			}
		} else { // Reply packet needs to be forawrded
			ttl--;
			if (ttl > 0 && MAX_ROUTE_LENGTH - ttl + 1 < path_len) {
				unpack_address(path[MAX_ROUTE_LENGTH - ttl + 1], next_hop);
			}

			// Build header
			network_send_pkt();
		}

	} else {
		fprintf(stderr, "ERROR: network_handler() received packet with invalid routing_packet_type\n");
	}

	// Restore old interrupt level
	set_interrupt_level(old_level);
}
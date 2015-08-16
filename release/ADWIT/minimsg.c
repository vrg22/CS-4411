/*
 *  Implementation of minimsgs and miniports.
 */
#include "minimsg.h"
#include "queue.h"
#include "stdlib.h"
#include "miniheader.h"
#include "stdio.h"
#include "string.h"
#include "minithread.h"
#include "synch.h"
#include "miniroute.h"

#define true 1;
#define false 0;

typedef int bool;

struct miniport {
	unsigned short this_port_number;
  	unsigned short send_port_number;
    bool is_bound;
  	network_address_t addr;
    semaphore_t port_sema;
	queue_t msg_queue;
};

typedef struct miniport miniport;


bool system_initialized;
queue_t bound_ports;
miniport_t *unbound_ports;
// queue_t unbound_ports;

short next_bound_port; // contains the next bound port number to use
miniport_t found_port;


/* performs any required initialization of the minimsg layer.
 */
void minimsg_initialize() {

    next_bound_port = 32768;

    unbound_ports = (miniport_t*)malloc(32768 * sizeof(miniport_t));
    // unbound_ports   = queue_new();
    bound_ports     = queue_new();
}

/* Iterator functions */
// void check_and_update_unbound(void *port_number, void *queue_element) {
// 	miniport_t current_unbound_port = (miniport_t) queue_element;
// 	// check if port of unbound is equal
// 	if (current_unbound_port->this_port_number == (short)(*(int *)port_number)) {
// 		found_port = current_unbound_port;
// 	}
// }

// void check_and_update_bound(void *dest_port_number, void *queue_element) {
//     miniport_t current_bound_port = (miniport_t) queue_element;
//     // check if dest port of bound is equal to given unbound port number
//     if (current_bound_port->send_port_number == (short)(*(int *)dest_port_number)) {
//         found_port = current_bound_port;
//     }
// }


// void check_network_locks(void *port_number, void *queue_element) {
// 	network_lock_t current_lock = (network_lock_t) queue_element;
//     // check if dest port of bound is equal to given unbound port number
//     if(current_lock->unbound_port->this_port_number == (short)(*(int *)port_number)) {
//         found_lock = current_lock;
//     }	
// }

// minithread_t find_network_thread(int unbound_port_number) {
// 	queue_t network_queue;

// 	network_queue = get_network_queue();
// 	found_lock = NULL;
// 	queue_iterate(network_queue, (func_t)check_network_locks, &unbound_port_number);
// 	return found_lock->thread;
// }


/* return 1 on success, 0 on failure (to find) */
int minimsg_find_and_update_unbound_port(int port_number, network_interrupt_arg_t *arg) {

	// queue_iterate(unbound_ports, (func_t)check_and_update_unbound, &port_number);

	miniport_t return_port;
	found_port = NULL;
	found_port = unbound_ports[port_number];

	if (found_port != NULL) {
		return_port = found_port;
		queue_append(return_port->msg_queue, arg);
		semaphore_V(return_port->port_sema);
		found_port = NULL;
		// printf("[SINK] ALERTED WAITING THREAD\n");
		return 1;
	} else {
		found_port = NULL;
		free(arg);
		printf("[SINK] PORT NOT FOUND\n");
		return 0;
	}
}

/* Creates an unbound port for listening. Multiple requests to create the same
 * unbound port should return the same miniport reference. It is the responsibility
 * of the programmer to make sure he does not destroy unbound miniports while they
 * are still in use by other threads -- this would result in undefined behavior.
 * Unbound ports must range from 0 to 32767. If the programmer specifies a port number
 * outside this range, it is considered an error.
 */
miniport_t miniport_create_unbound(int port_number) {
	if (!system_initialized) {
		minimsg_initialize();
		system_initialized = true;
	}

	found_port = NULL;
	// queue_iterate(unbound_ports, (func_t)check_and_update_unbound, &port_number);

	found_port = unbound_ports[port_number];

    if (found_port == NULL) {
        miniport_t unbound_port 			= (miniport_t)malloc(sizeof(miniport));
        unbound_port->this_port_number 		= (unsigned short) port_number;
        unbound_port->send_port_number 		= -1;
        unbound_port->is_bound 				= false;
        unbound_port->msg_queue 			= queue_new();
      	unbound_port->port_sema             = semaphore_create();
      	semaphore_initialize(unbound_port->port_sema,0);
        //CHECK RETURN OF 0 OR ONE
        // queue_append(unbound_ports, unbound_port);
        unbound_ports[port_number] 			= unbound_port;
        return unbound_port;
    } else {
        miniport_t return_unbound_port = found_port;
        return return_unbound_port;
    }
}

/* Creates a bound port for use in sending packets. The two parameters, addr and
 * remote_unbound_port_number together specify the remote's listening endpoint.
 * This function should assign bound port numbers incrementally between the range
 * 32768 to 65535. Port numbers should not be reused even if they have been destroyed,
 * unless an overflow occurs (ie. going over the 65535 limit) in which case you should
 * wrap around to 32768 again, incrementally assigning port numbers that are not
 * currently in use.
 */
miniport_t miniport_create_bound(network_address_t addr, int remote_unbound_port_number) {
	if (!system_initialized) {
		minimsg_initialize();
		system_initialized = true;
	}

	found_port = NULL;
	// queue_iterate(bound_ports,(func_t)check_and_update_bound,&remote_unbound_port_number);
    if (found_port == NULL) {
        // REMEMBER TO MOD
        miniport_t bound_port  		 	= (miniport_t)malloc(sizeof(miniport));
        next_bound_port					+= 1;
        if(next_bound_port > 65535) 	next_bound_port = 32768;
        bound_port->this_port_number 	= next_bound_port;
       	bound_port->send_port_number	= (unsigned short) remote_unbound_port_number;
       	// printf("CREATE PORT WITH REMOTE PORT %i\n", remote_unbound_port_number);
       	bound_port->is_bound 			= true;
       	network_address_copy(addr, bound_port->addr);
        return bound_port;
    } else {
        miniport_t return_bound_port = found_port;
        // found_port = NULL;
        return return_bound_port;
    }

}

/* Destroys a miniport and frees up its resources. If the miniport was in use at
 * the time it was destroyed, subsequent behavior is undefined.
 */
void miniport_destroy(miniport_t miniport) {
	if(miniport == NULL) {
		printf("NULL WTFF\n");
		return;
	}

	if(!miniport->is_bound) {
		queue_free(miniport->msg_queue);
		semaphore_destroy(miniport->port_sema);
	}

	free(miniport);
}

/* Sends a message through a locally bound port (the bound port already has an associated
 * receiver address so it is sufficient to just supply the bound port number). In order
 * for the remote system to correctly create a bound port for replies back to the sending
 * system, it needs to know the sender's listening port (specified by local_unbound_port).
 * The msg parameter is a pointer to a data payload that the user wishes to send and does not
 * include a network header; your implementation of minimsg_send must construct the header
 * before calling network_send_pkt(). The return value of this function is the number of
 * data payload bytes sent not inclusive of the header.
 */
int minimsg_send(miniport_t local_unbound_port, miniport_t local_bound_port, minimsg_t msg, int len) {
	mini_header_t header;
	network_address_t my_addr;
	char buffer[8];
	int i;
	int result;
	
	if(len > MAX_NETWORK_PKT_SIZE) {
		return 0;
	}

	// assingments
	for(i = 0;i < 8;i++) buffer[i] = 0;
	network_get_my_address(my_addr);
	
	// create and pack the header full of the information
	header = (mini_header_t)malloc(sizeof(struct mini_header));
	buffer[0] 											= PROTOCOL_MINIDATAGRAM;
	header->protocol									= buffer[0];

	pack_address(buffer, 								my_addr);
	for(i = 0;i < 8;i++) header->source_address[i] 		= buffer[i];
	
	pack_unsigned_short(buffer, 						local_unbound_port->this_port_number);
	for(i = 0;i < 2;i++) header->source_port[i]			= buffer[i];

	pack_address(buffer, 								local_bound_port->addr);
	for(i = 0;i < 8;i++) header->destination_address[i]	= buffer[i];
	
	pack_unsigned_short(buffer, 						local_bound_port->send_port_number);
	for(i = 0;i < 2;i++) header->destination_port[i]	= buffer[i];
	
	// call network send
	result = miniroute_send_pkt(local_bound_port->addr,
							  sizeof(struct mini_header), (char *)header,
							  len, (char *)msg);

	free(header);

	if(result < sizeof(mini_header_t) + len) {
    	return 0;
	}
	//printf("[SOURCE] SENDING \n[SOURCE] MESSAGE: %s[SOURCE] FROM SOURCE PORT %i TO DESTINATION PORT %i WITH PROTOCOL %i (%i bytes sent)\n", msg,local_bound_port->this_port_number,local_bound_port->send_port_number,header->protocol, result);
	return sizeof(msg); // RETURN SIZE OF MESSAGE (NOT HEADER)
}

/* Receives a message through a locally unbound port. Threads that call this function are
 * blocked until a message arrives. Upon arrival of each message, the function must create
 * a new bound port that targets the sender's address and listening port, so that use of
 * this created bound port results in replying directly back to the sender. It is the
 * responsibility of this function to strip off and parse the header before returning the
 * data payload and data length via the respective msg and len parameter. The return value
 * of this function is the number of data payload bytes received not inclusive of the header.
 */
int minimsg_receive(miniport_t local_unbound_port, miniport_t* new_local_bound_port, minimsg_t msg, int *len) {
	minimsg_t original_msg;
	mini_header_t header;
	// network_address_t source_addr;
	network_interrupt_arg_t *incoming_packet_p;
	miniport_t temp_port;
	int original_length;
	unsigned short source_port;
	void **temp;
	
	semaphore_P(local_unbound_port->port_sema);
	
	temp = (void **)malloc(sizeof(void *));
	// printf("[SINK] THREAD ALERTED\n");
	

	// CHECK IF DEQUEUE IS NULL OR FALSE
	// SHOULDN'T BE BECAUSE WE JUST ADDED TO QUEUE
	if(queue_dequeue(local_unbound_port->msg_queue, temp) == -1) return 0;
	incoming_packet_p = (network_interrupt_arg_t *)(*temp);


	// UNPACK AND RUMBLE
	original_length = incoming_packet_p->size - sizeof(struct mini_header);
	original_msg = (minimsg_t)((mini_header_t)(incoming_packet_p->buffer) + 1);
	header = (mini_header_t) (incoming_packet_p->buffer);
	
	

	// unpack_address(header->source_address, source_addr);
	source_port = unpack_unsigned_short(header->source_port);
	
	// printf("[SINK] addr %i %i port %i\n", source_addr[0], source_addr[1], source_port);
	temp_port = (miniport_t)miniport_create_bound(incoming_packet_p->sender, source_port);

	*new_local_bound_port = temp_port;
	// might need to free
	memcpy(msg, original_msg, original_length);
	free(temp);
	free(incoming_packet_p);

	*len = original_length;
	return original_length;
}


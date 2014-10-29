/*
 *  Implementation of minimsgs and miniports.
 */
#include "minimsg.h"

#define BOUND   0
#define UNBOUND 1

// Miniport variables and counters
// int unbound_ctr = UNBOUND_MIN_PORT_NUM;
int bound_ctr = BOUND_MIN_PORT_NUM;

miniport_t* ports = NULL; // Array of miniports for ports
semaphore_t bound_ports_free = NULL; // Number of bound miniports free
semaphore_t msgmutex = NULL; // Mutual exclusion semaphore


/* performs any required initialization of the minimsg layer. */
void minimsg_initialize() {
	int i;

	msgmutex = semaphore_create();
    semaphore_initialize(msgmutex, 1);

    bound_ports_free = semaphore_create();
    semaphore_initialize(bound_ports_free, BOUND_MAX_PORT_NUM - BOUND_MIN_PORT_NUM + 1);

    // Initialize ports array
	ports = (miniport_t*) malloc(BOUND_MAX_PORT_NUM * sizeof(miniport_t));

	if (ports == NULL) { // Fail if malloc() fails
      fprintf(stderr, "ERROR: minimsg_initialize() failed to malloc miniport_t array\n");
      return;
    }

    // Initialize each unbound port's data elements
    for (i = UNBOUND_MIN_PORT_NUM; i <= UNBOUND_MAX_PORT_NUM; i++) {
    	miniport_create_unbound(i);
    	ports[i]->u.unbound.incoming_data = queue_new();
    	ports[i]->u.unbound.datagrams_ready = semaphore_create();
    	semaphore_initialize(ports[i]->u.unbound.datagrams_ready, 0);
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
	miniport_t unbound_port;

	semaphore_P(msgmutex);

	// Ensure port_number is valid for this unbound miniport
	if (port_number < UNBOUND_MIN_PORT_NUM || port_number > UNBOUND_MAX_PORT_NUM) {
		fprintf(stderr, "ERROR: miniport_create_unbound() passed a bad port number\n");
		semaphore_V(msgmutex);
		return NULL;
	}

	// Allocate new port IF it does not already exist
	if (ports[port_number] == NULL) {
		unbound_port = malloc(sizeof(struct miniport));
		if (unbound_port == NULL) {
			fprintf(stderr, "ERROR: miniport_create_unbound() failed to malloc new miniport\n");
			semaphore_V(msgmutex);
			return NULL;
		}

		unbound_port->port_type = UNBOUND;
		unbound_port->port_num = port_number;
		unbound_port->u.unbound.incoming_data = queue_new();
		unbound_port->u.unbound.datagrams_ready = semaphore_create();
		semaphore_initialize(unbound_port->u.unbound.datagrams_ready, 0); // Counting semaphore

		ports[port_number] = unbound_port;
	}

	semaphore_V(msgmutex);
	
    return ports[port_number];
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
	miniport_t bound_port;

	/*// Ensure port_number is valid for this bound miniport
	if (port_number < BOUND_MIN_PORT_NUM || port_number > BOUND_MAX_PORT_NUM) {
		fprintf(stderr, "ERROR: miniport_create_bound() passed a bad port number\n");
		return NULL;
	}*/

	//Check validity of addr

	semaphore_P(msgmutex);

	semaphore_P(bound_ports_free); // Wait for a free bound port

	// Find next open bound port (guaranteed to exist by P() on bound_ports_free above)
	while (ports[bound_ctr] != NULL) {
		// bount_ctr = (bound_ctr + 1 - BOUND_MIN_PORT_NUM) % (BOUND_MAX_PORT_NUM - BOUND_MIN_PORT_NUM + 1) + BOUND_MIN_PORT_NUM;
		bound_ctr = (bound_ctr + 1 > BOUND_MAX_PORT_NUM) ? BOUND_MIN_PORT_NUM : (bound_ctr + 1); 
	}

	// Allocate new bound port
	bound_port = malloc(sizeof(struct miniport));
	if (bound_port == NULL) {
		fprintf(stderr, "ERROR: miniport_create_bound() failed to malloc new miniport\n");
		semaphore_V(msgmutex);
		return NULL;
	}

	bound_port->port_type = BOUND;
	bound_port->port_num = bound_ctr;
	network_address_copy(addr, bound_port->u.bound.remote_address);
	// bound_port->u.bound.remote_address = &addr;
	bound_port->u.bound.remote_unbound_port = remote_unbound_port_number;

	ports[bound_ctr] = bound_port;

	semaphore_V(msgmutex);

    return ports[bound_ctr];
}


/* Destroys a miniport and frees up its resources. If the miniport was in use at
 * the time it was destroyed, subsequent behavior is undefined.
 */
void miniport_destroy(miniport_t miniport) {
	semaphore_P(msgmutex);

	// Check for valid argument
	if (miniport == NULL) {
		fprintf(stderr, "ERROR: miniport_destroy() passed a NULL miniport argument\n");
		semaphore_V(msgmutex);
		return;
	}

	ports[miniport->port_num] = NULL; // Clear the miniport from the ports array
	if (miniport->port_type == BOUND) {
		semaphore_V(bound_ports_free); // Increment the bound port counting semaphore
	}

	//When to destroy? bounded->thread that used it terminates; unbounded->when last packet waits right there?
	free(miniport);

	semaphore_V(msgmutex);
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
	network_address_t dest, my_address;
	mini_header_t hdr;

	semaphore_P(msgmutex);

	// Check for valid arguments
	if (local_unbound_port == NULL) {
		fprintf(stderr, "ERROR: minimsg_send() passed a NULL local_unbound_port miniport argument\n");
		semaphore_V(msgmutex);
		return 0;
	}
	if (local_bound_port == NULL) {
		fprintf(stderr, "ERROR: minimsg_send() passed a NULL local_bound_port miniport argument\n");
		semaphore_V(msgmutex);
		return 0;
	}

	// Allocate new header for packet
	hdr = malloc(sizeof(struct mini_header));
	if (hdr == NULL) {	//Could not allocate header
		fprintf(stderr, "ERROR: minimsg_send() failed to malloc new mini_header\n");
		semaphore_V(msgmutex);
		return -1;
	}

	// Assemble packet header
	hdr->protocol = PROTOCOL_MINIDATAGRAM; // Protocol
	network_get_my_address(my_address);
	pack_address(hdr->source_address, my_address); // Source address
	pack_unsigned_short(hdr->source_port, local_bound_port->port_num); // Source port
	*dest = *(local_bound_port->u.bound.remote_address);
	pack_address(hdr->destination_address, dest); // Destination address
	pack_unsigned_short(hdr->destination_port, local_bound_port->u.bound.remote_unbound_port); // Destination port

	// Call network_send_pkt() from network.hdr
	if (network_send_pkt(dest, sizeof(hdr), (char*) hdr, sizeof(msg), msg) < 0) {
		fprintf(stderr, "ERROR: minimsg_send() failed to successfully execute network_send_pkt()\n");
		semaphore_V(msgmutex);
		return -1;
	}

	semaphore_V(msgmutex);

    return 0;
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
	unsigned short remote_port;
	char* buffer;
	network_address_t remote_receive_addr;
	network_interrupt_arg_t* packet = NULL;
	// mini_header_t header;

	semaphore_P(msgmutex);

	// Check for valid arguments
	if (local_unbound_port == NULL) {
		fprintf(stderr, "ERROR: minimsg_receive() passed a NULL local_unbound_port miniport argument\n");
		semaphore_V(msgmutex);
		return -1;
	}
	if (new_local_bound_port == NULL) {
		fprintf(stderr, "ERROR: minimsg_receive() passed a NULL new_local_bound_port miniport* argument\n");
		semaphore_V(msgmutex);
		return -1;
	}

	semaphore_V(msgmutex);

	semaphore_P(local_unbound_port->u.unbound.datagrams_ready); // Block until message arrives

	semaphore_P(msgmutex);

	// Obtain received message from miniport queue and extract header data
	if (queue_dequeue(local_unbound_port->u.unbound.incoming_data, (void*) packet) < 0) {
		fprintf(stderr, "ERROR: minimsg_send() failed to malloc new mini_header\n");
		semaphore_V(msgmutex);
		return -1;
	}

	// Extract header stuff
	buffer = packet->buffer;
	*len = packet->size;
	unpack_address(&buffer[1], remote_receive_addr);
	remote_port = unpack_unsigned_short(&buffer[9]);
	msg = (minimsg_t) &buffer[21];

	// Create new bound port
	*new_local_bound_port = miniport_create_bound(remote_receive_addr, remote_port);

 	//return number of bytes of payload actually received (drop stuff beyond max)

 	free(buffer);
 	free(packet);

 	semaphore_V(msgmutex);

    return sizeof(msg);
}
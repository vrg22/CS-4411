/*
 *	Implementation of minisockets.
 */
#include "minisocket.h"

minisocket_t* sockets = NULL; // Array of minisockets with each element representing a port
semaphore_t skt_mutex = NULL; // Mutual exclusion semaphore for manipulating socket data structures
int used_server_ports = 0; // Number of server ports in use
int used_client_ports = 0; // Number of client ports in use


/* Initializes the minisocket layer. */
void minisocket_initialize() {
	skt_mutex = semaphore_create();
    semaphore_initialize(skt_mutex, 1);

    // Initialize ports array
    sockets = (minisocket_t*) malloc((NUM_SERVER_PORTS + NUM_CLIENT_PORTS) * sizeof(minisocket_t));

	if (sockets == NULL) { // Fail if malloc() fails
      fprintf(stderr, "ERROR: minisocket_initialize() failed to malloc minisocket_t array\n");
      return;
    }
}

/* 
 * Listen for a connection from somebody else. When communication link is
 * created return a minisocket_t through which the communication can be made
 * from now on.
 *
 * The argument "port" is the port number on the local machine to which the
 * client will connect.
 *
 * Return value: the minisocket_t created, otherwise NULL with the errorcode
 * stored in the "error" variable.
 */
minisocket_t minisocket_server_create(int port, minisocket_error *error) {
	minisocket_t socket;
	// char* buffer;
	// int syn_done;
	int /*send_attempts, timeout,*/ received_ACK;
	// network_address_t dest, my_address;
	mini_header_reliable_t hdr; // Header for sending MSG_SYNACK message
	// network_interrupt_arg_t* packet = NULL;

	// semaphore_P(skt_mutex);

	// Check for available ports
	if (used_server_ports >= NUM_SERVER_PORTS) {
		fprintf(stderr, "ERROR: minisocket_server_create() unable to execute since no available ports exist\n");
		*error = SOCKET_NOMOREPORTS;
		// semaphore_V(skt_mutex);
		return NULL;
	}

	// Check for valid arguments
	if (port < SERVER_MIN_PORT || port > SERVER_MAX_PORT) {
		fprintf(stderr, "ERROR: minisocket_server_create() passed invalid port number\n");
		*error = SOCKET_INVALIDPARAMS;
		// semaphore_V(skt_mutex);
		return NULL;
	}
	if (sockets[port] != NULL) {
		fprintf(stderr, "ERROR: minisocket_server_create() passed port already in use\n");
		*error = SOCKET_PORTINUSE;
		// semaphore_V(skt_mutex);
		return NULL;
	}

	// Allocate new minisocket
	socket = malloc(sizeof(struct minisocket));
	if (socket == NULL) { // Could not allocate minisocket
		fprintf(stderr, "ERROR: minisocket_server_create() failed to malloc new minisocket\n");
		*error = SOCKET_OUTOFMEMORY;
		// semaphore_V(skt_mutex);
		return NULL;
	}

	// Set fields in minisocket
	socket->active = 0;
	socket->local_port = port;
	socket->datagrams_ready = semaphore_create();
	semaphore_initialize(socket->datagrams_ready, 0);
	socket->sending = semaphore_create();
	semaphore_initialize(socket->sending, 0);
	socket->receiving = semaphore_create();
	semaphore_initialize(socket->receiving, 0);
	socket->timeout = semaphore_create();
	semaphore_initialize(socket->timeout, 0);
	socket->wait_syn = semaphore_create();
	semaphore_initialize(socket->wait_syn, 0);
	socket->incoming_data = queue_new();
	socket->seqnum = 1;
	socket->acknum = 0;
	socket->alarm = NULL;

	sockets[port] = socket; // Add socket to socket ports array
	used_server_ports++; // Increment server-ports-in-use counter

	//Await SYN packet -> how much of this is done by network_handler???

	semaphore_P(socket->wait_syn);
	semaphore_initialize(socket->wait_syn, 0);

	/*syn_done = 0;
	while (!syn_done) {
		// semaphore_V(skt_mutex);
		semaphore_P(socket->datagrams_ready); // Block until a message is received (looking for a SYN to establish connection)
		// semaphore_P(skt_mutex);

		while (queue_dequeue(socket->incoming_data, (void**) &packet) >= 0 && !validate_packet(packet, MSG_SYN, 1, 0)) {
			// SEND FIN
		}

		// Check for message_type = MSG_SYN; (seq, ack) = (1, 0)
		if (packet && validate_packet(packet, MSG_SYN, 1, 0)) {
			syn_done = 1;
		}
	}*/
	
	// Received MSG_SYN with (seq, ack) = (1, 0); extract header stuff, update socket info
	/*socket->active = 1;
	socket->acknum++;
	buffer = packet->buffer; // Header and message data
	unpack_address(&buffer[1], socket->dest_address);
	socket->remote_port = unpack_unsigned_short(&buffer[9]);*/

	// free(packet);


	// Send SYNACK w/ 7 retries
	// Allocate new header for SYNACK packet
	hdr = malloc(sizeof(struct mini_header_reliable));
	if (hdr == NULL) {	// Could not allocate header
		fprintf(stderr, "ERROR: minisocket_server_create() failed to malloc new mini_header_reliable\n");
		*error = SOCKET_OUTOFMEMORY;
		// semaphore_V(skt_mutex);
		return NULL;
	}

	// Assemble packet header
	set_header(socket, hdr, MSG_SYNACK);

	// Send SYNACK packet, expect empty ACK packet
	received_ACK = retransmit_packet(socket, (char*) hdr, 0, NULL, error);

	// semaphore_V(skt_mutex);

	if (received_ACK == 1) {
		socket->alarm = NULL;		//No active retransmission alarm
		*error = SOCKET_NOERROR;
		return socket;
	} else {
		*error = SOCKET_RECEIVEERROR;
		return NULL;
	}
}



/*
 * Initiate the communication with a remote site. When communication is
 * established create a minisocket through which the communication can be made
 * from now on.
 *
 * The first argument is the network address of the remote machine.
 *
 * The argument "port" is the port number on the remote machine to which the
 * connection is made. The port number of the local machine is one of the free
 * port numbers.
 *
 * Return value: the minisocket_t created, otherwise NULL with the errorcode
 * stored in the "error" variable.
 */
minisocket_t minisocket_client_create(network_address_t addr, int port, minisocket_error *error) {
	minisocket_t socket;
	// char* buffer;
	int local_port = CLIENT_MIN_PORT;
	// int synack_done;
	int /*send_attempts, timeout,*/ received_SYNACK;
	// network_address_t dest, my_address;
	mini_header_reliable_t hdr; // Header for sending MSG_SYNACK message
	// network_interrupt_arg_t* packet = NULL;

	// semaphore_P(skt_mutex);

	// Check for available ports
	if (used_client_ports >= NUM_CLIENT_PORTS) {
		fprintf(stderr, "ERROR: minisocket_client_create() unable to execute since no available ports exist\n");
		*error = SOCKET_NOMOREPORTS;
		// semaphore_V(skt_mutex);
		return NULL;
	}

	// Check for valid arguments
	if (addr == NULL) {
		fprintf(stderr, "ERROR: minisocket_client_create() passed NULL network_address_t\n");
		*error = SOCKET_INVALIDPARAMS;
		// semaphore_V(skt_mutex);
		return NULL;
	}
	if (port < SERVER_MIN_PORT || port > SERVER_MAX_PORT) {
		fprintf(stderr, "ERROR: minisocket_client_create() passed invalid remote port number\n");
		*error = SOCKET_INVALIDPARAMS;
		// semaphore_V(skt_mutex);
		return NULL;
	}
	if (sockets[local_port] != NULL) {
		fprintf(stderr, "ERROR: minisocket_client_create() passed port already in use\n");
		*error = SOCKET_PORTINUSE;
		// semaphore_V(skt_mutex);
		return NULL;
	}

	// Allocate new minisocket
	socket = malloc(sizeof(struct minisocket));
	if (socket == NULL) { // Could not allocate minisocket
		fprintf(stderr, "ERROR: minisocket_client_create() failed to malloc new minisocket\n");
		*error = SOCKET_OUTOFMEMORY;
		// semaphore_V(skt_mutex);
		return NULL;
	}

	// Find next unused local port (guaranteed to exist due to counter and verification)
	while (local_port <= CLIENT_MAX_PORT && sockets[local_port] != NULL) {
		local_port++; 
	}
	if (sockets[local_port] != NULL) {
		fprintf(stderr, "ERROR: minisocket_client_create() ran out of available ports unexpectedly\n");
		*error = SOCKET_NOMOREPORTS;
		// semaphore_V(skt_mutex);
		return NULL;
	}

	// Set fields in minisocket
	socket->active = 1;
	socket->local_port = local_port;
	socket->remote_port = port;
	network_address_copy(addr, socket->dest_address);
	socket->datagrams_ready = semaphore_create();
	semaphore_initialize(socket->datagrams_ready, 0);
	socket->sending = semaphore_create();
	semaphore_initialize(socket->sending, 0);
	socket->receiving = semaphore_create();
	semaphore_initialize(socket->receiving, 0);
	socket->timeout = semaphore_create();
	semaphore_initialize(socket->timeout, 0);
	socket->wait_syn = semaphore_create();
	semaphore_initialize(socket->wait_syn, 0);
	socket->incoming_data = queue_new();
	socket->seqnum = 1;
	socket->acknum = 0;
	socket->alarm = NULL;

	sockets[local_port] = socket; // Add socket to socket ports array
	used_client_ports++; // Increment client-ports-in-use counter

	// Send MSG_SYN packet to server
	// Wait timeout, if no response, repeat 7 more times (7 retries)

	// Allocate new header for SYN packet
	hdr = malloc(sizeof(struct mini_header_reliable));
	if (hdr == NULL) {	// Could not allocate header
		fprintf(stderr, "ERROR: minisocket_client_create() failed to malloc new mini_header_reliable\n");
		*error = SOCKET_OUTOFMEMORY;
		// semaphore_V(skt_mutex);
		return NULL;
	}

	// Assemble packet header
	set_header(socket, hdr, MSG_SYN);

	// Send SYN packet, expect SYNACK packet
	received_SYNACK = retransmit_packet(socket, (char*) hdr, 0, NULL, error);

	// semaphore_V(skt_mutex);

	if (received_SYNACK) {
		*error = SOCKET_NOERROR;
		return socket;
	} else {
		socket->active = 0;
		*error = SOCKET_RECEIVEERROR;
		return NULL;
	}
}



/* 
 * Send a message to the other end of the socket.
 *
 * The send call should block until the remote host has ACKnowledged receipt of
 * the message.  This does not necessarily imply that the application has called
 * 'minisocket_receive', only that the packet is buffered pending a future
 * receive.
 *
 * It is expected that the order of calls to 'minisocket_send' implies the order
 * in which the concatenated messages will be received.
 *
 * 'minisocket_send' should block until the whole message is reliably
 * transmitted or an error/timeout occurs
 *
 * Arguments: the socket on which the communication is made (socket), the
 *            message to be transmitted (msg) and its length (len).
 * Return value: returns the number of successfully transmitted bytes. Sets the
 *               error code and returns -1 if an error is encountered.
 */
int minisocket_send(minisocket_t socket, minimsg_t msg, int len, minisocket_error *error) {
	int result, send_len;
	int bytes_sent = 0;
	mini_header_reliable_t header;

	// Check for valid arguments
	if (socket == NULL) {
		fprintf(stderr, "ERROR: minisocket_send() passed NULL minisocket_t\n");
		*error = SOCKET_INVALIDPARAMS;
		// semaphore_V(skt_mutex);
		return -1;
	}
	if (msg == NULL) {
		fprintf(stderr, "ERROR: minisocket_send() passed NULL minimsg_t\n");
		*error = SOCKET_INVALIDPARAMS;
		// semaphore_V(skt_mutex);
		return -1;
	}

	// Allocate new header
	header = malloc(sizeof(struct mini_header_reliable));
	if (header == NULL) {	// Could not allocate header
		fprintf(stderr, "ERROR: minisocket_send() failed to malloc new mini_header_reliable\n");
		*error = SOCKET_OUTOFMEMORY;
		// semaphore_V(skt_mutex);
		return -1;
	}

	semaphore_P(socket->sending);

	// Fragment long messages into smaller packets
	while (bytes_sent < len) {
		send_len = ((len - bytes_sent) > MAX_NETWORK_PKT_SIZE) ? MAX_NETWORK_PKT_SIZE : (len - bytes_sent); // Length of data to send in this packet
		set_header(socket, header, MSG_ACK);
		result = retransmit_packet(socket, (char*) header, send_len, msg + bytes_sent, error);

		if (result == 1) { // ACK received (packet send successfully)
			bytes_sent += send_len;
		} else if (result == 0) { // All timeout attempts failed - close connection
			semaphore_V(socket->sending);
			minisocket_close(socket);
			return -1;
		} else { // Generic failure
			semaphore_V(socket->sending);
			return -1;
		}
	}

	semaphore_V(socket->sending);

	return 0;
}


/*
 * Receive a message from the other end of the socket. Blocks until
 * some data is received (which can be smaller than max_len bytes).
 *
 * Arguments: the socket on which the communication is made (socket), the memory
 *            location where the received message is returned (msg) and its
 *            maximum length (max_len).
 * Return value: -1 in case of error and sets the error code, the number of
 *           bytes received otherwise
 */
int minisocket_receive(minisocket_t socket, minimsg_t msg, int max_len, minisocket_error *error) {
	int packets_remaining = 1;

	semaphore_P(socket->receiving); // Block until a message is received (looking for a SYN to establish connection)
	// Must acknowledge each packet upon arrival (provide info about losses)
	// Must keep track of packets it has already seen (handle duplicates)

	semaphore_P(socket->datagrams_ready);

	while (packets_remaining) {
		// do stuff
	}

	semaphore_V(socket->receiving);

	return 0;
}


/* Close a connection. If minisocket_close is issued, any send or receive should
 * fail.  As soon as the other side knows about the close, it should fail any
 * send or receive in progress. The minisocket is destroyed by minisocket_close
 * function.  The function should never fail.
 */
void minisocket_close(minisocket_t socket) {
	// TODO
}


/* HELPER METHODS */

int validate_packet(network_interrupt_arg_t* packet, char message_type, int seq_num, int ack_num) {
	char* buffer = packet->buffer;

	if (buffer[21] == message_type && unpack_unsigned_int(&buffer[22]) == seq_num && unpack_unsigned_int(&buffer[26]) == ack_num)
		return 1;
	else
		return 0;
}

void wait_for_arrival_or_timeout(semaphore_t sema, alarm_t* alarm, int timeout) {
	if (sema == NULL) {
		fprintf(stderr, "ERROR: wait_for_arrival_or_timeout() passed uninitialized semaphore\n");
		return;
	}
	*alarm = (alarm_t) register_alarm(timeout, (alarm_handler_t) semaphore_V, (void*) sema);
	semaphore_P(sema);
	deregister_alarm((alarm_id) (*alarm));
}

/* Used when we want to retransmit a given packet a certain number of times while a desired response has not been received 
	(relies on network_handler to get said response). Return -1 on Failure, 0 if Timed out, 1 if Received packet. */
int retransmit_packet(minisocket_t socket, char* hdr, int data_len, char* data, minisocket_error *error) {
	int send_attempts, timeout, received_next_packet;

	send_attempts = 0;
	timeout = INITIAL_TIMEOUT;
	received_next_packet = 0;

	while (send_attempts < MAX_SEND_ATTEMPTS && !received_next_packet) {
		if (network_send_pkt(socket->dest_address, sizeof(struct mini_header_reliable), hdr, data_len, data) < 0) {
			fprintf(stderr, "ERROR: retransmit_packet() failed to successfully execute network_send_pkt()\n");
			*error = SOCKET_SENDERROR;
			// semaphore_V(skt_mutex);
			return -1; // Failure
		}
		fprintf(stderr, "DEBUG: Sent %i with (syn = %i, ack = %i) attempt %i\n", ((mini_header_reliable_t) hdr)->message_type, unpack_unsigned_int(((mini_header_reliable_t) hdr)->seq_number), unpack_unsigned_int(((mini_header_reliable_t) hdr)->ack_number), send_attempts + 1);

		// Block here until timeout expires (and alarm is thus deregistered) or packet is received, deregistering the pending alarm		
		// CHECK: need to enforce mutual exclusion here?
		wait_for_arrival_or_timeout(socket->datagrams_ready, &(socket->alarm), timeout);	//CHECK: Is this the CORRECT semaphore to block on?

		// Q: What if you receive the ACK right here after timing out? -> Is that even a case we SHOULD or CAN handle?

		if (((alarm_t) socket->alarm)->executed) { // Timeout has been reached without ACK
			timeout *= 2;
			send_attempts++;
		} else { // ACK (or equivalent received)
			received_next_packet = 1;
			socket->alarm = NULL;		// No active retransmission alarm

			// socket->active = 1;		// CHECK: need this HERE if server? PROBABLY WRONG
		}
	}

	return received_next_packet;
}


/* Construct a (reliable) header to be sent by the given socket. */
void set_header(minisocket_t socket, mini_header_reliable_t hdr, char message_type) {
	network_address_t my_address;

	hdr->protocol = PROTOCOL_MINISTREAM; // Protocol
	network_get_my_address(my_address);
	pack_address(hdr->source_address, my_address); // Source address
	pack_unsigned_short(hdr->source_port, socket->local_port); // Source port
	pack_address(hdr->destination_address, socket->dest_address); // Destination address
	pack_unsigned_short(hdr->destination_port, socket->remote_port); // Destination port
	hdr->message_type = message_type; // Message type
	pack_unsigned_int(hdr->seq_number, socket->seqnum); // Sequence number
	pack_unsigned_int(hdr->ack_number, socket->acknum); // Acknowledgment number
}
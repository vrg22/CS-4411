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
	char* buffer;
	int syn_done;
	network_address_t dest, my_address;
	mini_header_reliable_t hdr; // Header for sending MSG_SYNACK message
	network_interrupt_arg_t* packet = NULL;


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
	socket->incoming_data = queue_new();
	socket->seqnum = 1;
	socket->acknum = 0;

	sockets[port] = socket; // Add socket to socket ports array
	used_server_ports++; // Increment server-ports-in-use counter
	

	syn_done = 0;
	while (!syn_done) {
		// semaphore_V(skt_mutex);
		semaphore_P(socket->datagrams_ready); // Block until a message is received (looking for a SYN to establish connection)
		// semaphore_P(skt_mutex);

		while (queue_dequeue(socket->incoming_data, (void**) &packet) >= 0 && !validate_packet(packet, MSG_SYN, 1, 0)) {
			// SEND FIN
		}

		// Check for message_type = MSG_SYN; (seq, ack) = (1, 0)
		if (validate_packet(packet, MSG_SYN, 1, 0)) {
			syn_done = 1;
		}
	}
	
	// Received MSG_SYN with (seq, ack) = (1, 0); extract header stuff
	buffer = packet->buffer; // Header and message data
	unpack_address(&buffer[1], socket->dest_address);
	socket->remote_port = unpack_unsigned_short(&buffer[9]);

	free(packet);

	// Send SYNACK w/ 7 retries
	// Construct header

	// Allocate new header for SYNACK packet
	hdr = malloc(sizeof(struct mini_header_reliable));
	if (hdr == NULL) {	//Could not allocate header
		fprintf(stderr, "ERROR: minisocket_server_create() failed to malloc new mini_header_reliable\n");
		*error = SOCKET_OUTOFMEMORY;
		// semaphore_V(skt_mutex);
		return NULL;
	}

	// Assemble packet header
	hdr->protocol = PROTOCOL_MINISTREAM; // Protocol
	network_get_my_address(my_address);
	pack_address(hdr->source_address, my_address); // Source address
	pack_unsigned_short(hdr->source_port, local_bound_port->port_num); // Source port
	network_address_copy(local_bound_port->u.bound.remote_address, dest);
	pack_address(hdr->destination_address, dest); // Destination address
	pack_unsigned_short(hdr->destination_port, local_bound_port->u.bound.remote_unbound_port); // Destination port

	// Call network_send_pkt() from network.hdr
	if (network_send_pkt(dest, sizeof(struct mini_header), (char*) hdr, len, msg) < 0) {
		fprintf(stderr, "ERROR: minimsg_send() failed to successfully execute network_send_pkt()\n");
		semaphore_V(msgmutex);
		return -1;
	}

	// semaphore_V(msgmutex);

    return 0;


	// Wait for ACK OR compatible msg (i.e. (2, 1)?

	return socket;
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
	// Send MSG_SYN packet to server
	// Wait timeout, if no response, repeat 7 more times (7 retries)
	// If still no response, return SOCKET_NOSERVER

	// If receive MSG_SYNACK, send MSG_ACK to server
	// Connection now open

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

	// Send message w/ MSG_ACK as msg type
	// Block until we receive acknowledgment or until we give up (in which case return partial result or error)

	// Must set timeout to receive ACK and resend packet if no response

	Server:
	//If got MSG_SYN, then return MSG_SYNACK

	Client:
	//Send Msg_syn packet -> try once, retry 7 times, if no response, return SOCKET_NOSERVER error
	//ACK the SYNACK
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
	semaphore_P(socket->datagrams_ready); // Block until a message is received (looking for a SYN to establish connection)
	// Must acknowledge each packet upon arrival (provide info about losses)
	// Must keep track of packets it has already seen (handle duplicates)
}

/* Close a connection. If minisocket_close is issued, any send or receive should
 * fail.  As soon as the other side knows about the close, it should fail any
 * send or receive in progress. The minisocket is destroyed by minisocket_close
 * function.  The function should never fail.
 */
void minisocket_close(minisocket_t socket) {

}

int validate_packet(network_interrupt_arg_t* packet, char message_type, int seq_num, int ack_num) {
	char* buffer = packet->buffer;

	if (&buffer[21] == message_type && unpack_unsigned_int(&buffer[22]) == seq_num && unpack_unsigned_int(&buffer[26]) == ack_num)
		return 1;
	else
		return 0;
}
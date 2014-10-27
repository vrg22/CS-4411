/*
 *  Implementation of minimsgs and miniports.
 */
#include "minimsg.h"

#define BOUND   0		//Change?
#define UNBOUND 1

//Miniport variables and counters
semaphore_t boundports;
int unbound_ctr = 0;
int bound_ctr = 32768;


struct miniport { 
 	char port_type; 
 	int port_num;
	union { 
 		struct { 
 			queue_t incoming_data; 
 			semaphore_t datagrams_ready;
 		} unbound; 
 	
		struct { 
 			network_address_t remote_address; 
 			int remote_unbound_port; 
 		} bound; 
	} u;
};

// struct miniport
// {
// 	int type;	//0 for bound, 1 for unbound
// 	int port_num;
// 
// 	//BOUNDED ATTRIBUTES
// 	int dest_port_num;
// 	//miniport_addr_t dest_addr
// 
// 	//UNBOUNDED ATTRIBUTES
// 	//queue_t pckts;	 //Is this only for recv side?
// 	queue_t blocked_threads; //Is this only for recv side? (unbound?)
// 	queue_t messages; //Is this only for recv side?
// };



/* performs any required initialization of the minimsg layer. */
void minimsg_initialize() {
	//Array of 65536 miniports (is this a reasonable idea for fast check???) -> seems bad
		//-How else to quickly know whether or not a given miniport exists and quickly access it?

	//Semaphore to limit number of bound ports at a time? (ie, should a bound port be reserved for a thread?)
	
	//?
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

	//For RECEIVING data
	//Have fields to allow:
		// -messages to be queued on the port
		// -threads to block waiting for messages

	//Ensure port_number is valid for this unbound miniport
	if (!(port_number >= 0 && port_number <= 32767)){
		fprintf(stderr, "ERROR: miniport_create_unbound() was passed a bad port number\n");
		return NULL;	//Invalid unbound port number
	}

	
	//NEED TO ADD LOOKUP IF PORT ALREADY EXISTS!!!
	//


	//Allocate new port IF it does not already exist
	unbound_port = malloc(sizeof(struct miniport));
	if (unbound_port == NULL){
		fprintf(stderr, "ERROR: miniport_create_unbound() failed to malloc new miniport\n");
        return NULL;
	}
	unbound_port->port_type = UNBOUND;
	unbound_port->port_num = port_number;
	unbound_port->u.unbound.incoming_data = queue_new();
	unbound_port->u.unbound.datagrams_ready = semaphore_create();
	semaphore_initialize(unbound_port->u.unbound.datagrams_ready, 0); //Counting semaphore

    return unbound_port;
}


/* Creates a bound port for use in sending packets. The two parameters, addr and
 * remote_unbound_port_number together specify the remote's listening endpoint.
 * This function should assign bound port numbers incrementally between the range
 * 32768 to 65535. Port numbers should not be reused even if they have been destroyed,
 * unless an overflow occurs (ie. going over the 65535 limit) in which case you should
 * wrap around to 32768 again, incrementally assigning port numbers that are not
 * currently in use.
 */
miniport_t
miniport_create_bound(network_address_t addr, int remote_unbound_port_number)
{
	miniport_t bound_port;

	//For SENDING data
	//Carefully assign port numbers to OUR miniport! -> bound corresponds to unbound

	//Ensure remote_unbound_port_number is a valid unbound port number
	if (!(remote_unbound_port_number >= 0 && remote_unbound_port_number <= 32767)){
		fprintf(stderr, "ERROR: miniport_create_bound() was passed a bad remote port number\n");
		return NULL;	//Invalid remote port number passed
	}

	//Check validity of addr
	//-How?


	//NEED TO ADD LOOKUP IF PORT ALREADY EXISTS!!!
	//


	//Allocate new port IF it does not already exist??? -> should you P() on a counting sema to ensure there are NEVER more than 65536?
	bound_port = malloc(sizeof(struct miniport));
	if (bound_port == NULL){
		fprintf(stderr, "ERROR: miniport_create_bound() failed to malloc new miniport\n");
        return NULL;
	}
	bound_port->port_num = bound_ctr;
	bound_ctr = (bound_ctr + 1 > 65536) ? 32768 : (bound_ctr + 1); 
	
	//Set destination port number
	bound_port->u.bound.remote_address = addr;
	bound_port->u.bound.remote_unbound_port = remote_unbound_port_number;

    return bound_port;
}


/* Destroys a miniport and frees up its resources. If the miniport was in use at
 * the time it was destroyed, subsequent behavior is undefined.
 */
void miniport_destroy(miniport_t miniport) {
	//Check argument
	if (miniport == NULL){
		fprintf(stderr, "ERROR: miniport_destroy() was passed a bad argument\n");
		return;
	}
	
	//Make SURE that the miniport is not in use at this time
	//If Bounded: 
	//If Unbounded:
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
	int sent;
	network_address_t dest;
	mini_header_t hdr;

	//Assemble header identifying destination
	hdr = malloc(sizeof(struct mini_header));
	if (hdr == NULL){	//Could not allocate header
		fprintf(stderr, "ERROR: minimsg_send() failed to malloc new miniport\n");
		return -1;
	}

	//Call network_send_pkt() from network.h
	dest = local_bound_port->;
	sent = network_send_pkt(network_address_t dest_address,
                 len, hdr,
                 int  data_len, char * data);

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
	//CHECK THAT PARAMETERS ARE VALID!

 	//Examine header
 		-decode to determine which miniport it has been sent to

 	//Determine destination
 	//Enqueue packet in right place (port?)
 	//Wake up any threads that may be blocked waiting for a packet to arrive
 		-if none, then message queued at miniport until a receive is performed
 		-if multiple, use round-robin pattern

 	//Allow receiver to reply!
 		-create bound port here
 		-create corresponding unbound port (referring to sender port)

 	//msg should contain the data payload
 	//len should point to the data length
 	//return number of bytes of payload actually received (drop stuff beyond max)

    return 0;
}



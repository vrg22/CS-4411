/*
 * minithread.c:
 *      This file provides a few function headers for the procedures that
 *      you are required to implement for the minithread assignment.
 *
 *      EXCEPT WHERE NOTED YOUR IMPLEMENTATION MUST CONFORM TO THE
 *      NAMING AND TYPING OF THESE PROCEDURES.
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include "minithread.h"

#include "miniheader.h"
#include "minimsg.h"
#include "minisocket.h"
#include "read_private.h"


/*
 * A minithread should be defined either in this file or in a private
 * header file.  Minithreads have a stack pointer with which to make procedure
 * calls, a stackbase which points to the bottom of the procedure
 * call stack, the ability to be enqueued and dequeued, and any other state
 * that you feel they must have.
 */


semaphore_t mutex = NULL;

/* LOCAL SCHEDULER VARIABLES */
multilevel_queue_t run_queue = NULL;            // The running multilevel feedback queue
int system_run_level = -1;                      // The level of the currently running process
float prob_level[4] = {0.5, 0.25, 0.15, 0.1};   // Probability of selecting thread at given level
int quanta_level[4] = {1, 2, 4, 8};             // Quanta assigned for each level

minithread_t globaltcb;                         // Main TCB that contains the "OS" thread
int thread_ctr = 0;                             // Counts created threads (used for ID assignment)
minithread_t current;                           // Keeps track of the currently running minithread
queue_t zombie_queue;                           // Keeps dead threads for cleanup. Cleaned by kernel TCB when size exceeds limit
int zombie_limit = 5;                           // Limit on length of zombie queue  


/* minithread functions */

minithread_t minithread_fork(proc_t proc, arg_t arg) {
	minithread_t tcb = minithread_create(proc, arg);

	// Check for argument errors
	if (tcb == NULL) {
		fprintf(stderr, "ERROR: minithread_fork() failed to create new minithread_t\n");
		return NULL;
	}
	if (proc == NULL) { // Fail if process pointer is NULL
		fprintf(stderr, "ERROR: minithread_fork() passed a NULL process pointer\n");
		return NULL;
	}

	minithread_start(tcb);

	return tcb;
}

minithread_t minithread_create(proc_t proc, arg_t arg) {
	minithread_t tcb;

	if (proc == NULL) { // Fail if process pointer is NULL
		fprintf(stderr, "ERROR: minithread_create() passed a NULL process pointer\n");
		return NULL;
	}

	tcb = malloc(sizeof(struct minithread));

	if (tcb == NULL) { // Fail if malloc() fails
		fprintf(stderr, "ERROR: minithread_create() failed to malloc new TCB\n");
		return NULL;
	}

	// Set TCB properties
	tcb->id = ++thread_ctr;
	tcb->dead = 0;
	tcb->func = proc;
	tcb->arg = arg;
	tcb->run_level = 0; // Add new thread to highest level in run_queue
	tcb->quanta_left = quanta_level[0];
	
	// Set up TCB stack
	minithread_allocate_stack(&(tcb->stackbase), &(tcb->stacktop)); // Allocate new stack
	minithread_initialize_stack(&(tcb->stacktop), proc, arg, /*&*/ minithread_exit, (arg_t) tcb); // Initialize stack with proc & cleanup functions
	
	return tcb;
}

/* */
minithread_t minithread_self() {
	return current;
}

/* */
int minithread_id() {
	return current->id;
}

/* Take the current process off the run_queue. Can choose to put in a wait_queue. 
	 Regardless, switch over to OS to now decide what to do. */
void minithread_stop() {
	minithread_t tcb_old;

	set_interrupt_level(DISABLED);        //CHECK!!!
	tcb_old = current;

	// Context switch to OS
	current = globaltcb;
	minithread_switch(&(tcb_old->stacktop), &(globaltcb->stacktop));
}

void minithread_start(minithread_t t) {
	interrupt_level_t old_level = set_interrupt_level(DISABLED);        //CHECK!!!

	semaphore_P(mutex);
	// Place at level 0 by default
	// current->run_level = 0;
	if (multilevel_queue_enqueue(run_queue, t->run_level, t) < 0) {
	fprintf(stderr, "ERROR: minithread_yield() failed to append thread to end of level 0 in run_queue\n");
	return;
	}
	semaphore_V(mutex);

	set_interrupt_level(old_level);
}

void minithread_yield() {
	minithread_t tcb_old;

	set_interrupt_level(DISABLED);
	tcb_old = current;

	semaphore_P(mutex);
	/* Move current process to end of its current level in run_queue */
	if (multilevel_queue_enqueue(run_queue, current->run_level, current) < 0) {
	fprintf(stderr, "ERROR: minithread_yield() failed to append current process to end of its level in run_queue\n");
	return;
	}
	semaphore_V(mutex);

	//Context switch to OS
	current = globaltcb;
	minithread_switch(&(tcb_old->stacktop), &(globaltcb->stacktop));
}

/*
 * This is the clock interrupt handling routine.
 * You have to call minithread_clock_init with this
 * function as parameter in minithread_system_initialize
 */
void clock_handler(void* arg) {
	elem_q* iter;
	alarm_t alarm;
	void (*func)();
	void (*argument);
	minithread_t tcb_old;
	interrupt_level_t old_level = set_interrupt_level(DISABLED); // Disable interrupts

	tcb_old = current;

	clk_count++; // Increment clock count

	if (alarm_queue == NULL) { // Ensure alarm_queue has been initialized
	 alarm_queue = queue_new();
	}
	iter = alarm_queue->head;

	// While next alarm deadline has passed
	while (iter && (iter->next != alarm_queue->head) && (((alarm_t)(iter->data))->deadline <= ((unsigned long long) clk_count) * clk_period)) {
	alarm = (alarm_t) iter->data;
	if (alarm->executed != 1) {      //Only if we haven't yet processed this alarm
		func = alarm->func;
		argument = alarm->arg;
		func(argument);
		alarm->executed = 1;
	}
	iter = iter->next;   // Bump alarm pointer
	}

	// Process last alarm_queue element
	if (iter && (((alarm_t)(iter->data))->deadline <= ((unsigned long long) clk_count) * clk_period)) {
	alarm = (alarm_t) iter->data;
	if (alarm->executed != 1) {      //Only if we haven't yet processed this alarm
		func = alarm->func;
		argument = alarm->arg;
		func(argument);
		alarm->executed = 1;
	}
	}


	// Track non-privileged process quanta
	if (current != globaltcb) {  // Applies only to non-OS threads
	(current->quanta_left)--;    // Do we guarantee that this only happens AFTER the current thread has run >= 1 quanta?
	
	// Time's Up
	if (current->quanta_left == 0) {
		current->run_level = (current->run_level + 1) % 4;   // Choose to wrap around processes to have priority "refreshed"
		current->quanta_left = quanta_level[current->run_level];
		multilevel_queue_enqueue(run_queue, current->run_level, current);
		
		current = globaltcb;
		// system_run_level = -1;
		minithread_switch(&(tcb_old->stacktop), &(globaltcb->stacktop));  //Context switch to OS to choose next process
	}
	}

	set_interrupt_level(old_level); // Restore old interrupt level
}

/*
 * Initialization.
 *
 *      minithread_system_initialize:
 *       This procedure should be called from your C main procedure
 *       to turn a single threaded UNIX process into a multithreaded
 *       program.
 *
 *       Initialize any private data structures.
 *       "Create the idle thread."
 *       Fork the thread which should call mainproc(mainarg)
 *       Start scheduling.
 *
 */
void minithread_system_initialize(proc_t mainproc, arg_t mainarg) {
	// Create "OS"/kernel TCB
	globaltcb = (minithread_t) malloc(sizeof(struct minithread));
	if (globaltcb == NULL) { // Fail if malloc() fails
	fprintf(stderr, "ERROR: minithread_system_initialize() failed to malloc kernel TCB\n");
	return;
	}

	// Allocate stack
	minithread_allocate_stack(&(globaltcb->stackbase), &(globaltcb->stacktop));

	// Create mutex for shared-state accesses
	mutex = semaphore_create();
	semaphore_initialize(mutex, 1);

	// Create multilevel-feedback queue
	if (run_queue == NULL) run_queue = multilevel_queue_new(4);
	// system_run_level = -1;

	// Create zombie queue for dead threads
	if (zombie_queue == NULL) zombie_queue = queue_new();

	// Create and schedule first minithread                 
	current = minithread_fork(mainproc, mainarg);         //CHECK FOR INVARIANT!!!!!

	/* Set up clock and alarms */
	minithread_clock_init(clk_period, (interrupt_handler_t) &clock_handler);
	alarm_queue = queue_new();

	// Initialize the network and related resources
	network_initialize((network_handler_t) &network_handler);
	minimsg_initialize();
	minisocket_initialize();
	miniroute_initialize();
	miniterm_initialize();

	// Initialize Disk
	disk_initialize(&disk);
	install_disk_handler(disk_handler);

	set_interrupt_level(ENABLED);

	// OS Code
	while (1) {
	// Periodically free up zombie queue
	if (queue_length(zombie_queue) == zombie_limit) {
		queue_iterate(zombie_queue, (func_t) minithread_deallocate_func, NULL);     //deallocate all threads in zombie queue
		//queue_free(zombie_queue);                                 //NOTE: Would NOT need to again do "queue_new" for zombie_thread
		zombie_queue = queue_new();                               //IFF we've written queue_free such that it does not make zombie_queue NULL 
	}

	if (multilevel_queue_length(run_queue) > 0) {
		minithread_next(globaltcb); // Select next ready process
	}
	}  
}

/*
 * minithread_sleep_with_timeout(int delay)
 *      Put the current thread to sleep for [delay] milliseconds
 */
void minithread_sleep_with_timeout(int delay) {
	semaphore_t alert = semaphore_create();
	semaphore_initialize(alert, 0);
	register_alarm(delay, (alarm_handler_t) semaphore_V, (void*) alert);
	semaphore_P(alert);
}

/* Pick next process to run. SHOULD ONLY run in OS MODE */
void minithread_next(minithread_t self) {

	//Make the OS scheduler probabilistic rather than just selecting from the next level down
	int nxt_lvl;
	float val = (float)rand() / RAND_MAX;
	// float prob = 0;
	// int i = 0;

	/*while (i < run_queue->num_levels) {
	prob += prob_level[i];
	if (val < prob) {
		nxt_lvl = i;
	}
	i++;
	}*/

	interrupt_level_t old_level = set_interrupt_level(DISABLED); // Disable interrupts

	if (val < prob_level[0])       // Pri 0
		nxt_lvl = 0;
	else if (val < (prob_level[0] + prob_level[1]))  // Pri 1
		nxt_lvl = 1;
	else if (val < (prob_level[0] + prob_level[1] + prob_level[2]))  // Pri 2
		nxt_lvl = 2;
	else   //Pri 3
		nxt_lvl = 3;

	// fprintf(stderr, "%d\n", nxt_lvl); // DEBUG!!!

	system_run_level = multilevel_queue_dequeue(run_queue, nxt_lvl, (void**) &current); // Set new run_level

	if (current == NULL) {
	fprintf(stderr, "ERROR: minithread_next() attempted to context switch to NULL current thread pointer\n");
	}

	minithread_switch(&(self->stacktop), &(current->stacktop)); // Context switch to next ready process

	set_interrupt_level(old_level); // Restore old interrupt level
}

/*
* Thread finishing function (used as finalproc when thread terminates)
*/
int minithread_exit(arg_t arg) {
	// Need to disable to modify zombie queue while changing "current"
	set_interrupt_level(DISABLED);

	((minithread_t) arg)->dead = 1;   //do we need this? if using zombie_queue, probably not...
	queue_append(zombie_queue, current);
	
	// Context switch to OS/kernel
	current = globaltcb;
	// minithread_switch(&(tcb_old->stacktop), &(globaltcb->stacktop));
	minithread_switch(&(((minithread_t) arg)->stacktop), &(globaltcb->stacktop));
	return 0;
}

/* Wake up a thread. */
int minithread_wake(minithread_t thread) {
	return multilevel_queue_enqueue(run_queue, thread->run_level, thread);    //CHECK!!!    //NOT currently thread-safe
}

/* Deallocate a thread. */
void minithread_deallocate(minithread_t thread) {
	free(thread);
}

/* Deallocate a thread => wrapper for queue_iterate use. */
void minithread_deallocate_func(void* null_arg, void* thread) {
	minithread_deallocate((minithread_t) thread);
}


/*
 * This is the network interrupt packet handling routine.
 * You have to call network_initialize with this function as parameter in minithread_system_initialize
 */
void network_handler(network_interrupt_arg_t* pkt) {
	network_address_t my_addr, destination, reply_dest, next_hop, temp, dest_addr;
	cache_elem_t dest_elem;
	int /*id,*/ ttl, path_len;
	int i, j;
	char* buffer;
	char path[MAX_ROUTE_LENGTH][8];
	char new_path[MAX_ROUTE_LENGTH][8];
	char routing_packet_type;
	routing_header_t routing_hdr;

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
	// id = unpack_unsigned_int(&buffer[9]); 		// NEED TO USE
	ttl = unpack_unsigned_int(&buffer[13]);
	path_len = unpack_unsigned_int(&buffer[17]); // Path length of received packet
	for (i = 0; i < path_len; i++) {	// Extract path from packet
		for (j = 0; j < 8; j++) {
			path[i][j] = buffer[21 + 8*i + j];
		}
	}
	//buffer = &buffer[21 + 8*MAX_ROUTE_LENGTH]; // Bump to start of payload

	if (routing_packet_type == ROUTING_DATA) {
		if (network_compare_network_addresses(destination, my_addr)) { // I am final destination
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
			} else { //Handle as TCP-reliable datagram (protocol == PROTOCOL_MINISTREAM)
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
		} else { // Forward packet on to next hop
			ttl--;
			if (ttl > 0 && MAX_ROUTE_LENGTH - ttl + 1 < path_len) {
				unpack_address(path[MAX_ROUTE_LENGTH - ttl + 1], next_hop);
			}

			// Allocate new routing header
			routing_hdr = malloc(sizeof(struct routing_header));
			if (routing_hdr == NULL) {	// Could not allocate header
				fprintf(stderr, "ERROR: network_handler() failed to malloc routing header\n");
				return;
			}

			// Build routing_hdr with updated fields
			routing_hdr->routing_packet_type = ROUTING_DATA;
			pack_address(routing_hdr->destination, destination);
			pack_unsigned_int(routing_hdr->ttl, ttl);
			pack_unsigned_int(routing_hdr->path_len, path_len);
			for (i = 0; i < path_len/*MAX_ROUTE_LENGTH*/; i++) {
				for (j = 0; j < 8; j++) {
					routing_hdr->path[i][j] = path[i][j];
				}
			}

			// Unicast send to next hop
			network_send_pkt(next_hop, sizeof(struct routing_header), (char*) routing_hdr, (pkt->size - sizeof(struct routing_header)), buffer);
		}
	} else if (routing_packet_type == ROUTING_ROUTE_DISCOVERY) {
		if (network_compare_network_addresses(destination, my_addr)) { // I am final destination
			unpack_address(&path[0][0], reply_dest); // Set final destination for reply packet
			
			// Add ourselves to end of path
			pack_address(&path[path_len][0], my_addr);
			path_len++;

			// Reverse path
			for (i = path_len; i > 0; i--) {
				for (j = 0; j < 8; j++) {
					new_path[path_len - i][j] = path[i-1][j];
				}
			}

			unpack_address(&path[1][0], next_hop); // Set destination address for reply packet's next hop
			
			// Allocate new routing header
			routing_hdr = malloc(sizeof(struct routing_header));
			if (routing_hdr == NULL) {	// Could not allocate header
				fprintf(stderr, "ERROR: network_handler() failed to malloc routing header\n");
				return;
			}

			// Build routing_hdr with updated fields
			routing_hdr->routing_packet_type = ROUTING_ROUTE_REPLY;
			pack_address(routing_hdr->destination, reply_dest);
			pack_unsigned_int(routing_hdr->ttl, MAX_ROUTE_LENGTH);
			pack_unsigned_int(routing_hdr->path_len, path_len);
			for (i = 0; i < path_len/*MAX_ROUTE_LENGTH*/; i++) {
				for (j = 0; j < 8; j++) {
					routing_hdr->path[i][j] = new_path[i][j]; // Header should have the REVERSED path
				}
			}

			// Unicast send to reply dest
			network_send_pkt(next_hop, sizeof(struct routing_header), (char*) routing_hdr, 0, NULL); // No relevant data
		} else { // Discovery packet needs to be rebroadcast
			ttl--;
			if (ttl > 0) {
				// Check that I have not gotten this before
				for (i = 0; i < path_len; i++) {
					unpack_address(&path[i][0], temp);
					if (network_compare_network_addresses(temp, my_addr)) {
						// fprintf(stderr, "ERROR: network_handler() got a discovery pkt that reached me before\n");
						return;
					}
				}

				// Add ourselves to end of path
				pack_address(&path[path_len][0], my_addr);
				path_len++;
				
				// Allocate new routing header
				routing_hdr = malloc(sizeof(struct routing_header));
				if (routing_hdr == NULL) {	// Could not allocate header
					fprintf(stderr, "ERROR: network_handler() failed to malloc expanded header\n");
					return;
				}

				// Build routing_hdr with updated fields
				routing_hdr->routing_packet_type = ROUTING_ROUTE_DISCOVERY;
				pack_address(routing_hdr->destination, destination);
				pack_unsigned_int(routing_hdr->ttl, ttl);
				pack_unsigned_int(routing_hdr->path_len, path_len);
				for (i = 0; i < path_len/*MAX_ROUTE_LENGTH*/; i++) {
					for (j = 0; j < 8; j++) {
						routing_hdr->path[i][j] = path[i][j];
					}
				}

				// Rebroadcast packet w/ updated params
				network_bcast_pkt(sizeof(struct routing_header), (char*) routing_hdr, 0, NULL); // No relevant data
			}
		}
	} else if (routing_packet_type == ROUTING_ROUTE_REPLY) {
		if (network_compare_network_addresses(destination, my_addr)) { // I am final destination
			dest_elem = cache_table_get(cache, destination);
			if (dest_elem == NULL) {
				fprintf(stderr, "ERROR: network_handler() found a reply for null cache destination\n");
				return;
			}
			if (dest_elem->path[0][0] == 0) { // Only update cache entry's path if not already completed
				if (dest_elem->reply != NULL && dest_elem->reply->executed == 0) { // Timout has not yet occured and reply has not already been received
					deregister_alarm(dest_elem->reply);
					dest_elem->reply = NULL;

					// Reverse path
					for (i = path_len; i >= 0; i--) {
						for (j = 0; j < 8; j++) {
							dest_elem->path[path_len - i][j] = path[i][j];
						}
					}
					dest_elem->path_len = path_len; // Update path length

					// Create cache entry expiration alarm
					dest_elem->expire = register_alarm(3000, (alarm_handler_t) remove_cache_entry, (void*) dest_elem);

					semaphore_V(dest_elem->timeout);
				}
			} else {
				fprintf(stderr, "ERROR: network_handler() found a reply for non-null cache path\n");
				return;
			}
		} else { // Reply packet needs to be forwarded
			ttl--;
			if (ttl > 0 && MAX_ROUTE_LENGTH - ttl + 1 < path_len) {
				unpack_address(path[MAX_ROUTE_LENGTH - ttl + 1], next_hop);
			}

			// Allocate new routing header
			routing_hdr = malloc(sizeof(struct routing_header));
			if (routing_hdr == NULL) {	// Could not allocate header
				fprintf(stderr, "ERROR: network_handler() failed to malloc routing header\n");
				return;
			}

			// Build routing_hdr with updated fields
			routing_hdr->routing_packet_type = ROUTING_ROUTE_REPLY;
			pack_address(routing_hdr->destination, destination);
			pack_unsigned_int(routing_hdr->ttl, ttl);
			pack_unsigned_int(routing_hdr->path_len, path_len);
			for (i = 0; i < path_len/*MAX_ROUTE_LENGTH*/; i++) {
				for (j = 0; j < 8; j++) {
					routing_hdr->path[i][j] = path[i][j];
				}
			}

			// Unicast forward the reply to next hop
			network_send_pkt(next_hop, sizeof(struct routing_header), (char*) routing_hdr, 0, NULL); // No relevant data

		}

	} else {
		fprintf(stderr, "ERROR: network_handler() received packet with invalid routing_packet_type\n");
	}

	// Restore old interrupt level
	set_interrupt_level(old_level);
}

void remove_cache_entry(cache_elem_t entry) {
	int result;

	deregister_alarm(entry->expire);
	result = cache_table_remove(cache, entry->dest);

	if (result < 0) {
		fprintf(stderr, "ERROR: remove_cache_entry() couldn't remove cache table entry\n");
	}
}

void disk_handler(disk_interrupt_arg_t arg) {
	return;
}
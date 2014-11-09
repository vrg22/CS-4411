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
#include "interrupts.h"
#include "minithread.h"
#include "queue.h"
#include "synch.h"
#include "alarm.h"
#include "multilevel_queue.h"

#include "miniheader.h"
#include "minimsg.h"

#include <assert.h>

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

  // Debug statements to check if clock_handler() is working
  /*if (clk_count % 10 != 0 && clk_count % 1 == 0) fprintf(stderr, "Tick\n");
  if (clk_count % 10 == 0) fprintf(stderr, "Tock\n");*/

  if (alarm_queue == NULL) { // Ensure alarm_queue has been initialized
    alarm_queue = queue_new();
  }
  iter = alarm_queue->head;
  while (iter && (((alarm_t)(iter->data))->deadline <= clk_count * clk_period)) { // While next alarm deadline has passed
    alarm = (alarm_t) iter->data;
    func = alarm->func;
    argument = alarm->arg;
    func(argument);
    alarm->executed = 1;
    deregister_alarm((alarm_id) alarm);
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
  // minithread_t tcb_old;

  // tcb_old = current;

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
  network_address_t /*src_addr,*/ dest_addr, my_addr;   //Q: for my_addr, should it just be a global var?
  unsigned short /*src_port,*/ dest_port; //Can we pass this to an int? In fact, should we just set this to an int?
  char* buffer;     //data; //Good style for extra data pointer?
  //char protocol;
  //int length;

  interrupt_level_t old_level = set_interrupt_level(DISABLED); // Disable interrupts

  // fprintf(stderr, "network_handler()\n");

  // Set my system's address
  network_get_my_address(my_addr);

  // Basic packet parameters
  buffer = pkt->buffer;
  //length = pkt->size;   //Header size + Data size (ie num of char elts that matter?)

  // Extract req'd info from packet header
  //protocol = buffer[0];   //buffer = &buffer[1];  //Advance pointer

  //unpack_address(&buffer[1], src_addr); //Where packet originally came from      //Make sure this works
  //src_port = unpack_unsigned_short(&buffer[9]);

  unpack_address(&buffer[11], dest_addr); //Ultimate packet destination: may need to send it away if it doesn't match my address
  dest_port = unpack_unsigned_short(&buffer[19]);

  // //Create pointer to data payload
  // data = &buffer[21];

  // Makes sense to never destroy unbound ports? -> YES: WE INITIALIZE UNBOUND PORTS IN MINIMSG INITIALIZE
  // enqueue at unbound port
  // network_address_copy(pkt->sender, addr);
  
  //TODO:
    // -Am I the final dest?
    //   -If NO, call send on this packet
    //   -If YES, need to place msg at appropriate locally unbound port
    //     -Q: what to do if port is not already existing? Do I (as interrupt handler) MAKE the receive port?
    //       Or do I MAKE the port? -> STATE ASSUMPTION HERE

  
  if (network_compare_network_addresses(dest_addr, my_addr) != 0) {  // This packet is meant for me
    if (ports[dest_port] != NULL) {     //Locally unbound port exists
      if (ports[dest_port]->u.unbound.incoming_data != NULL) {  //Queue at locally unbound port has been initialized
        //Put PTR TO ENTIRE PACKET (type: network_interrupt_arg_t*) in the queue at that port
        queue_append(ports[dest_port]->u.unbound.incoming_data, /*(void*)*/ pkt);   //(minimsg_t) buffer, data;
        semaphore_V(ports[dest_port]->u.unbound.datagrams_ready);   // V on semaphore
      } else
        fprintf(stderr, "queue not set");
    } else
      fprintf(stderr, "dest port doesn't exist\n");
    //Check if port already exists before calling receive, if doesnt, drop packet
    // Why drop? B/C calling create_unbound in the handler would require semaphore_P(msgmutex), which is not allowed in an interrupt handler
  } else {
    fprintf(stderr, "address not for me\n");//I am NOT the final packet destination
    // SHOULD I JUST DROP THE PACKET???
    
    // DONT CALL SEND HERE! SHOULD BE DONE BY THE PROGRAMMER // minimsg_send(miniport_t local_unbound_port, miniport_t local_bound_port, minimsg_t msg, int len);
    // Do we assume send creates a local bound port to send?
  }

  //Free packet after processing? -> NO: since I'm enqueueing POINTER to packet itself
  //free(pkt);

  set_interrupt_level(old_level); // Restore old interrupt level
}
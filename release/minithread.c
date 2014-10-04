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

#include <assert.h>

/*
 * A minithread should be defined either in this file or in a private
 * header file.  Minithreads have a stack pointer with which to make procedure
 * calls, a stackbase which points to the bottom of the procedure
 * call stack, the ability to be enqueued and dequeued, and any other state
 * that you feel they must have.
 */

/* LOCAL SCHEDULER VARIABLES */

semaphore_t mutex = NULL;

multilevel_queue_t run_queue = NULL;            // The running multilevel feedback queue
int system_run_level = -1;                     // The level of the currently running process
double prob_level[4] = {0.5, 0.25, 0.15, 0.1};  // Probability of selecting thread at given level       //NOTE: double or float or long?
int quant_level[4] = {1, 2, 4, 8};              // Quanta assigned for each level

minithread_t globaltcb;                         // Main TCB that contains the "OS" thread
int thread_ctr = 0;                             // Counts created threads. Used for ID assignment
minithread_t current;                           // Keeps track of the currently running minithread
queue_t zombie_queue;                           // Keeps dead threads for cleanup. Cleaned by kernel TCB when size exceeds limit
int zombie_limit = 5;                           // Limit on length of zombie queue  
                                                           //CHECK arbitrary selection of zombie_limit!!!

/* CLOCK VARIABLES */
int clk_period = SECOND;        // Clock interrupt period           //NOTE: reduce your clock period to 100 ms
long clk_count = 0;             // Running count of clock interrupts
// queue_t alarm_queue = NULL;  // Queue containing alarms (soonest deadline at head of queue)


/* minithread functions */

minithread_t minithread_fork(proc_t proc, arg_t arg) {
    minithread_t tcb = minithread_create(proc, arg);
    if (tcb == NULL) return NULL;
    minithread_start(tcb);
    return tcb;
}

minithread_t minithread_create(proc_t proc, arg_t arg) {
    minithread_t tcb;

    if (proc == NULL) { // Fail if process pointer is NULL
      printf("ERROR: minithread_create() passed a NULL process pointer\n");
      return NULL;
    }

    tcb = malloc(sizeof(struct minithread));

    if (tcb == NULL) { // Fail if malloc() fails
      printf("ERROR: minithread_create() failed to malloc new TCB\n");
      return NULL;
    }

    // Set TCB properties
    tcb->id = ++thread_ctr;
    tcb->func = proc;
    tcb->arg = arg;
    tcb->dead = 0;
    tcb->run_level = 0;   //Default, add tcb to top-level queue
    tcb->quant_left = quant_level[0];
    tcb->priviliged = 0;  //Default privilege level
    
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
  semaphore_P(mutex);
  current = globaltcb;
  semaphore_V(mutex);

  // minithread_next(current); // Jump to next ready process

  // minithread_t tcb_old;
  //
  // /* Remove current process from head of run_queue */
  // if (queue_dequeue(run_queue, (void**) &tcb_old) < 0) {
  //   printf("ERROR: minithread_stop() failed to dequeue current process from head of run_queue");
  //   return;
  // }
}

void minithread_start(minithread_t t) {
  // Place at level 0 by default
  if (multilevel_queue_enqueue(run_queue, 0, t) < 0) {
    printf("ERROR: minithread_yield() failed to append current process to end of its level in run_queue");
    return;
  }
}

void minithread_yield() {
  /* Move current process to end of its current level in run_queue */
  if (multilevel_queue_enqueue(run_queue, current->run_level, current) < 0) {
    printf("ERROR: minithread_yield() failed to append current process to end of its level in run_queue");
    return;
  }

  jump back to globaltcb, which will then pick the next ready process
  // minithread_next(current); // Jump to next ready process

  // minithread_t tcb_old;
  //
  // /* Move current process (at head of run_queue) to tail of run_queue */
  // if (queue_dequeue(run_queue, (void**) &tcb_old) < 0) {
  //   printf("ERROR: minithread_yield() failed to dequeue current process from head of run_queue");
  //   return;
  // }
  // if (queue_append(run_queue, tcb_old) < 0) {
  //   printf("ERROR: minithread_yield() failed to append current process to end of run_queue");
  //   return;
}

/*
 * This is the clock interrupt handling routine.
 * You have to call minithread_clock_init with this
 * function as parameter in minithread_system_initialize
 */
void clock_handler(void* arg) {
  interrupt_level_t old_level = set_interrupt_level(DISABLED); // Disable interrupts
  clk_count++; // Increment clock count
    // if (clk_count % 10 == 0) printf("Count");

  //Track non-privileged process quanta
  if (current->privileged == 0) {  //Applies only to non-OS threads
    (current->quant_left)--;    //Do we guarantee that this only happens AFTER the current thread has run >= 1 quanta?
    
    //Time's Up
    if (current->quant_left == 0){
      current->run_level = (current->run_level + 1) % 4;   //Choose to wrap around processes to have priority "refreshed"
      current->quant_left = quant_level[current->run_level];
      multilevel_queue_enqueue(run_queue, current->run_level, current);
      current = globaltcb;  //NECESSARY?
      minithread_switch(&(current->stacktop), &(globaltcb->stacktop));  //Context switch to OS to choose next process
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
    printf("ERROR: minithread_system_initialize() failed to malloc kernel TCB\n");
    return;
  }
  globaltcb->priviliged = 1;    //Give kernel TCB the privilige bit
  minithread_allocate_stack(&(globaltcb->stackbase), &(globaltcb->stacktop));

  // Create mutex for shared-state accesses
  mutex = semaphore_create();

  // Create multilevel-feedback queue
  if (run_queue == NULL) run_queue = multilevel_queue_new(4);
  system_run_level = 0;

  // Create zombie queue for dead threads
  if (zombie_queue == NULL) zombie_queue = queue_new();

  // Create and schedule first minithread                 
  current = minithread_fork(mainproc, mainarg);         //CHECK FOR INVARIANT!!!!!

  /* Set up clock and alarms */
  minithread_clock_init(clk_period, (interrupt_handler_t) &clock_handler);
  set_interrupt_level(ENABLED);
  //alarm_queue = queue_new();

  // OS Code
  while (1) {    
    // Periodically free up zombie queue
    if (queue_length(zombie_queue) == zombie_limit){
      queue_iterate(zombie_queue, (func_t) minithread_deallocate_func, NULL);     //deallocate all threads in zombie queue
      queue_free(zombie_queue);                                 //NOTE: Would NOT need to again do "queue_new" for zombie_thread
      zombie_queue = queue_new();                               //IFF we've written queue_free such that it does not make zombie_queue NULL 
    }

    // Select next ready process
    minithread_next(globaltcb);
  }
}

/* Pick next process to run */
void minithread_next(minithread_t self) {

  //Make the OS scheduler probabilistic rather than just selecting from the next level down
  int nxt_lvl;
  double val = (double)rand() / RAND_MAX;

  if (val < prob_level[0])       // Pri 0
      nxt_lvl = 0;
  else if (val < (prob_level[0] + prob_level[1]))  // Pri 1
      nxt_lvl = 1;
  else if (val < (prob_level[0] + prob_level[1] + prob_level[2]))  // Pri 2
      nxt_lvl = 2;
  else   //Pri 3
      nxt_lvl = 3;

  //Set new run_level
  system_run_level = multilevel_queue_dequeue(run_queue, nxt_lvl, (void**) &current);

  if (system_run_level >= 0) {
    // Context switch to next ready process
    minithread_switch(&(self->stacktop), &(current->stacktop));
  }

}

/*
* Thread finishing function.    "Finalproc"
*/
int minithread_exit(arg_t arg) {
  //NOTE: ensure that clock interrupt doesnt mess things up if it occurs here
  //IE, if we go into clock interrupt and are preempted, then we would have to be enqueued again somewhere,
  //but we're almost dead. then, it may take a while to finally clean me up -> is this acceptable?
  //Is it OK or not to disable interrupts for the time here to put me on the zombie queue?
  set_interrupt_level(DISABLED);

  ((minithread_t) arg)->dead = 1;   //do we need this? if using zombie_queue, probably not...
  queue_append(zombie_queue, current);
  
  // Context switch to OS/kernel
  current = globaltcb;                                  //NECESSARY?
  minithread_switch(&(((minithread_t) arg)->stacktop), &(globaltcb->stacktop));
  return 0;
}

/* Wake up a thread. */
int minithread_wake(minithread_t thread) {
  return multilevel_queue_enqueue(run_queue, thread->run_level, thread);
  // return queue_append(run_queue, thread);
}

/* Deallocate a thread. */
void minithread_deallocate(minithread_t thread) {
  free(thread);
}

/* Deallocate a thread => wrapper for queue_iterate use. */
void minithread_deallocate_func(void* null_arg, void* thread) {
  minithread_deallocate((minithread_t) thread);
}

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
// queue_t run_queue = NULL;                    // The running queue of tcbs

multilevel_queue_t run_queue = NULL;            // The running multilevel feedback queue
int current_run_level = -1;                     // The level of the currently running process
float prob_level[4] = {0.5, 0.25, 0.15, 0.1};   // Probability of selecting thread at given level
int quant_level[4] = {1, 2, 4, 8};              // Quanta assigned for each level

minithread_t globaltcb;                         // Main TCB that contains the "OS" thread
int thread_ctr = 0;                             // Counts created threads. Used for ID assignment
minithread_t current;                           // Keeps track of the currently running minithread

/* CLOCK VARIABLES */
int clk_period  = SECOND;       // Clock interrupt period           //NOTE: reduce your clock period to 100 ms
int clk_quantum = 0;            // Scheduler time-quantum unit      //NOTE: set to 'clk_period' somewhere!!!
long clk_count = 0;             // Running count of clock interrupts
// queue_t alarm_queue;         // Queue containing alarms (soonest deadline at head of queue)


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
    
    // Set up TCB stack
    minithread_allocate_stack(&(tcb->stackbase), &(tcb->stacktop)); // Allocate new stack
    minithread_initialize_stack(&(tcb->stacktop), proc, arg, /*&*/ minithread_exit, (arg_t) tcb); // Initialize stack with proc & cleanup functions
    
    return tcb;
}

/* */
minithread_t minithread_self() {
    return current;

    // return (minithread_t) (run_queue->head->data);
}

/* */
int minithread_id() {
    return current->id;

    // minithread_t tcb;
    // if (queue_length(run_queue) == 0) return -1; // No running thread
    
    // tcb = (minithread_t) (run_queue->head->data);
    // return tcb->id;
}

/* "current" is running; Do not enqueue it to the running queue. 
    Simply run the next valid thread.   */
void minithread_stop() {
  minithread_next(current); // Jump to next ready process

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
  
  // if (queue_append(run_queue, t) == -1) return;
}

/* */
void minithread_yield() {
  /* Move current process to end of its current level in run_queue */
  if (multilevel_queue_enqueue(run_queue, current->run_level, current) < 0) {
    printf("ERROR: minithread_yield() failed to append current process to end of its level in run_queue");
    return;
  }

  minithread_next(current); // Jump to next ready process

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
  // Create multilevel-feedback queue
  if (run_queue == NULL) run_queue = multilevel_queue_new(4);
  current_run_level = 0;

  // Create "OS"/kernel TCB
  globaltcb = (minithread_t) malloc(sizeof(struct minithread));
  if (globaltcb == NULL) { // Fail if malloc() fails
    printf("ERROR: minithread_system_initialize() failed to malloc kernel TCB\n");
    return;
  }
  minithread_allocate_stack(&(globaltcb->stackbase), &(globaltcb->stacktop));

  // Create and schedule first minithread                 //CHECK FOR INVARIANT!!!!!!!
  current = minithread_fork(mainproc, mainarg);

  /* Set up clock and alarms */
  minithread_clock_init(clk_period, (interrupt_handler_t) &clock_handler);
  set_interrupt_level(ENABLED);
  //alarm_queue = queue_new();

  // OS Code
  while (1) {    
    // Destroy preceding process if it was marked as dead
    if ((current != NULL) && (current->dead == 1)) {
      // printf("Deallocating...\n");
      minithread_deallocate(current);
      // printf("Deallocated!\n");
      current = globaltcb; // Update current thread pointer
    }
    //else {//????
    // Select next ready process
    // current = globaltcb; //HACK!!!
    minithread_next(globaltcb);
    //}
  }
    
    //OLD CODE:

    // Create ready queue
    // if (run_queue == NULL) run_queue = queue_new();
    //
    // if (queue_length(run_queue) > 0) { // Only take action if ready queue is not empty
    //   if ((current != NULL) && (current->dead == 1)) { // Destory former process if it was marked as dead
    //     // Remove current process from head of run_queue
    //     if (queue_dequeue(run_queue, (void**) &current) < 0) {
    //       printf("ERROR: minithread_system_initialize() failed to dequeue current process from head of run_queue\n");
    //     }
    //     minithread_deallocate(current);
    //     current = globaltcb; // Update current thread pointer
    //   } else {
    //     minithread_next(globaltcb); // Select next ready process
    //   }
    // }
}

/* Pick next process to run */
void minithread_next(minithread_t self) {
  // int quantum;

  //NOTE: Need to modify s.t. the starting point of OS scheduler is probabilistic, NOT just the next level down!!!
  //Enqueue old guy at the next level he should be located in!!!

  //Set new run_level
  current_run_level = multilevel_queue_dequeue(run_queue, current_run_level, (void**) &current);
  if (current_run_level < 0) {
    // printf("ERROR: minithread_next() called on empty run_queue\n");   //No longer error because we don't let them access multilevel queue size
    current_run_level = 0;    //CHECK! We need to make sure that 
    minithread_switch(&(self->stacktop), &(globaltcb->stacktop));
  } else {
    //Determine new time quantum
    // quantum = clk_quantum * quant_level[current_run_level];
    minithread_switch(&(self->stacktop), &(current->stacktop)); // Context switch to next ready process
  }

  // if (queue_length(run_queue) > 0) {
  //   current = run_queue->head->data; // Update current thread pointer to next ready process
  //   minithread_switch(&(self->stacktop), &(current->stacktop)); // Context switch to next ready process
  // } else {
  //   printf("ERROR: minithread_next() called on empty run_queue");
  //   minithread_switch(&(self->stacktop), &(globaltcb->stacktop));
  // }

  //OLD: Run FIFO from running queue
  /*while (queue_length(run_queue) > 0) {          
    tcb = (minithread_t) (run_queue->head->data);
    minithread_switch(&(globaltcb->stacktop), &(tcb->stacktop));
    if (queue_dequeue(run_queue, (void**) &tcb) == -1) return;
  }*/
}

/*
* Freeing function.    "Finalproc"
*/
int minithread_exit(arg_t arg) {
  ((minithread_t) arg)->dead = 1;
  
  // Context switch to OS/kernel
  minithread_switch(&(((minithread_t) arg)->stacktop), &(globaltcb->stacktop));
  return 0;
}

/* */
int minithread_wake(minithread_t thread) {
  return multilevel_queue_enqueue(run_queue, thread->run_level, thread);
  // return queue_append(run_queue, thread);
}

/* */
void minithread_deallocate(minithread_t thread) {
  free(thread);
}

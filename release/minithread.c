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

#include <assert.h>

/*
 * A minithread should be defined either in this file or in a private
 * header file.  Minithreads have a stack pointer with which to make procedure
 * calls, a stackbase which points to the bottom of the procedure
 * call stack, the ability to be enqueued and dequeued, and any other state
 * that you feel they must have.
 */

/* LOCAL SCHEDULER VARIABLES */

queue_t run_queue = NULL;   // The running queue of tcbs
minithread_t globaltcb;     // Main TCB that contains the "OS" thread
int thread_ctr = 0;         // Counts created threads. Used for ID assignment
minithread_t current;       // Keeps track of the currently running minithread


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
    
    // Set up TCB stack
    minithread_allocate_stack(&(tcb->stackbase), &(tcb->stacktop)); // Allocate new stack
    minithread_initialize_stack(&(tcb->stacktop), proc, arg, /*&*/ minithread_exit, (arg_t) tcb); // Initialize stack with proc & cleanup functions
    
    return tcb;
}

minithread_t minithread_self() { // Return current instead?
    return (minithread_t) (run_queue->head->data);
}

int minithread_id() {               
    minithread_t tcb;
    if (queue_length(run_queue) == 0) return -1; // No running thread
    
    tcb = (minithread_t) (run_queue->head->data);
    return tcb->id;
}

void minithread_stop() {
  minithread_t tcb_old;

  /* Remove current process from head of run_queue */
  if (queue_dequeue(run_queue, (void**) &tcb_old) < 0) {
    printf("ERROR: minithread_stop() failed to dequeue current process from head of run_queue");
    return;
  }
	minithread_next(tcb_old); // Jump to next ready process
}

void minithread_start(minithread_t t) {
	if (queue_append(run_queue, t) == -1) return;
}

void minithread_yield() {
  minithread_t tcb_old;

  /* Move current process (at head of run_queue) to tail of run_queue */
  if (queue_dequeue(run_queue, (void**) &tcb_old) < 0) {
    printf("ERROR: minithread_yield() failed to dequeue current process from head of run_queue");
    return;
  }
  if (queue_append(run_queue, tcb_old) < 0) {
    printf("ERROR: minithread_yield() failed to append current process to end of run_queue");
    return;
  }

  minithread_next(tcb_old); // Jump to next ready process
}

/*
 * This is the clock interrupt handling routine.
 * You have to call minithread_clock_init with this
 * function as parameter in minithread_system_initialize
 */
void 
clock_handler(void* arg)
{

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
    if (run_queue == NULL) run_queue = queue_new(); // Create ready queue
    
    // Create "OS"/kernel TCB
    globaltcb = (minithread_t) malloc(sizeof(struct minithread));
    if (globaltcb == NULL) { // Fail if malloc() fails
      printf("ERROR: minithread_system_initialize() failed to malloc kernel TCB\n");
      return;
    }
    minithread_allocate_stack(&(globaltcb->stackbase), &(globaltcb->stacktop));

    // Create and schedule first minithread
    current = minithread_fork(mainproc, mainarg);

    while (1) {
      if (queue_length(run_queue) > 0) { // Only take action if ready queue is not empty
        if ((current != NULL) && (current->dead == 1)) { // Destory former process if it was marked as dead
          // Remove current process from head of run_queue
          if (queue_dequeue(run_queue, (void**) &current) < 0) {
            printf("ERROR: minithread_system_initialize() failed to dequeue current process from head of run_queue\n");
          }
          minithread_deallocate(current);
          current = globaltcb; // Update current thread pointer
        } else {
          minithread_next(globaltcb); // Select next ready process
        }
      }
    }
  }

/*
 * sleep with timeout in milliseconds
 */

void minithread_sleep_with_timeout(int delay) {

}

    //Run FIFO from running queue
    /*while (queue_length(run_queue) > 0) {          
      tcb = (minithread_t) (run_queue->head->data);
      minithread_switch(&(globaltcb->stacktop), &(tcb->stacktop));
      if (queue_dequeue(run_queue, (void**) &tcb) == -1) return;
    }*/

void minithread_next(minithread_t self) {

  if (queue_length(run_queue) > 0) {
    current = run_queue->head->data; // Update current thread pointer to next ready process
    minithread_switch(&(self->stacktop), &(current->stacktop)); // Context switch to next ready process
  } else {
    printf("ERROR: minithread_next() called on empty run_queue");
    minithread_switch(&(self->stacktop), &(globaltcb->stacktop));
  }
}

void minithread_deallocate(minithread_t thread) {
  free(thread);
}

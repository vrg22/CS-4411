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

queue_t run_queue = NULL;   //The running queue of tcbs
minithread_t globaltcb;     //global tcb used for correct context switching

int thread_ctr = 0;         //Counts created threads. Used for ID assignment



/* minithread functions */

minithread_t minithread_fork(proc_t proc, arg_t arg) {               //CHECK!
    minithread_t tcb = minithread_create(proc, arg);
    if (tcb == NULL) return NULL;
    minithread_start(tcb);
    return tcb;
}

minithread_t minithread_create(proc_t proc, arg_t arg) {
    minithread_t tcb;

    //if (proc == NULL) return NULL;  //fail if process pointer is NULL
    if (proc == NULL) {
      printf("something's really wrong...\n");
      return NULL;
    }   //fail if process pointer is NULL

    tcb = malloc(sizeof(struct minithread));
    //if (tcb == NULL) return NULL;   //malloc failed
    if (tcb == NULL) {
      printf("something's really wrong...\n");
      return NULL;
    }   //malloc failed

    tcb->id = ++thread_ctr;
    tcb->func = proc;
    tcb->arg = arg;
    
    minithread_allocate_stack(&(tcb->stackbase), &(tcb->stacktop)); 						//allocate fresh stack
    minithread_initialize_stack(&(tcb->stacktop), proc, arg, /*&*/ minithread_exit, (arg_t) tcb);		//initialize stack w/ proc & cleanup function
    																						//CHECK arg to minithread exit!
    return tcb;
}

minithread_t minithread_self() {						//NEED TO COMPLETE!
    return (minithread_t)0;
}

int minithread_id() {               
    minithread_t tcb;
    if (queue_length(run_queue) == 0) return -1;			//No running thread
    
    tcb = (minithread_t) (run_queue->head->data);
    return tcb->id;
}

void minithread_stop() {
	//Prevent calling thread from running again w/o explicit OS permission
	minithread_t tcb_old, tcb_new;
	tcb_old = NULL;
  if (queue_dequeue(run_queue, (void**) &tcb_old) == -1) return;
  tcb_new = (minithread_t) (run_queue->head->data);
  minithread_switch(&(tcb_old->stacktop), &(tcb_new->stacktop));
  //DO WE WANT TO FREE?
}

void minithread_start(minithread_t t) {
	if (queue_append(run_queue, t) == -1) return;			//WORKS WITH APPEND, fails with PREPEND
}

void minithread_yield() {
   	minithread_t tcb_old, tcb_new;
   	if (queue_dequeue(run_queue, (void**) &tcb_old) == -1) return;
    if (queue_append(run_queue, tcb_old) == -1) return;
   	tcb_new = (minithread_t) (run_queue->head->data);
   	minithread_switch(&(tcb_old->stacktop), &(tcb_new->stacktop));
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
    minithread_t tcb;

    //Create running queue
    if (run_queue == NULL) run_queue = queue_new();
    
    //Create context-switching tcb
    globaltcb = (minithread_t) malloc(sizeof(struct minithread));
    if (globaltcb == NULL) return;
    minithread_allocate_stack(&(globaltcb->stackbase), &(globaltcb->stacktop));

    //Create and schedule first tcb
    minithread_fork(mainproc, mainarg);

    //Run FIFO from running queue
    while (queue_length(run_queue) > 0) {          
      tcb = (minithread_t) (run_queue->head->data);
      minithread_switch(&(globaltcb->stacktop), &(tcb->stacktop));
      if (queue_dequeue(run_queue, (void**) &tcb) == -1) return;
    }
    
}

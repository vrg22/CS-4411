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

queue_t run_queue = NULL;

int thread_ctr = 0; //Need to initialize



/* minithread functions */

minithread_t
minithread_fork(proc_t proc, arg_t arg) {               //CHECK!
    minithread_t tcb = minithread_create(proc, arg);
    if (tcb == NULL) return NULL;   //create failed
    minithread_start(tcb);
    return tcb;
}

minithread_t
minithread_create(proc_t proc, arg_t arg) {
    minithread_t tcb;

    if (proc == NULL) return NULL;  //fail if process pointer is NULL

    tcb = malloc(sizeof(struct minithread));
    if (tcb == NULL) return NULL;   //malloc failed

    tcb->id = thread_ctr++;
    tcb->func = proc;
    tcb->arg = arg;
    
    return tcb;
}

minithread_t
minithread_self() {
    return (minithread_t)0;
}

int
minithread_id() {               //-1 for no running???
    minithread_t tcb = (minithread_t) ((run_queue->head)->data);
    return tcb->id;
}

void
minithread_stop() {
}

void
minithread_start(minithread_t t) {		//???
    proc_t func = t->func;
    if (queue_prepend(run_queue, t) == -1) return;   //couldn't add to queue  ???
    func(t->arg);
    return;
}

void
minithread_yield() {
    //
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
 *       "Create the idle thread.""
 *       Fork the thread which should call mainproc(mainarg)
 *       Start scheduling.
 *
 */
void
minithread_system_initialize(proc_t mainproc, arg_t mainarg) {          //?????
    if (run_queue == NULL) run_queue = queue_new();

    minithread_fork(mainproc, mainarg);         //creates and schedules
    //run_queue
    //if (run_queue->len)
    
}



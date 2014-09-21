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

minithread_t minithread_fork(proc_t proc, arg_t arg) {               //CHECK!
    minithread_t tcb = minithread_create(proc, arg);
    if (tcb == NULL){ printf("something's really wrong...\n"); return NULL; };   //create failed
    minithread_start(tcb);
    return tcb;
}

minithread_t minithread_create(proc_t proc, arg_t arg) {
    minithread_t tcb;

    //if (proc == NULL) return NULL;  //fail if process pointer is NULL
    if (proc == NULL){ printf("something's really wrong...\n"); return NULL; };   //fail if process pointer is NULL

    tcb = malloc(sizeof(struct minithread));
    //if (tcb == NULL) return NULL;   //malloc failed
    if (tcb == NULL){ printf("something's really wrong...\n"); return NULL; };   //malloc failed

    tcb->id = thread_ctr++;
    tcb->func = proc;
    tcb->arg = arg;
    
    minithread_allocate_stack(&(tcb->stackbase), &(tcb->stacktop)); 						//allocate fresh stack
    minithread_initialize_stack(&(tcb->stacktop), proc, arg, /*&*/ minithread_exit, NULL);		//initialize stack w/ proc & cleanup function
    																						//CHECK arg to minithread exit!
    return tcb;
}

minithread_t minithread_self() {						//COMPLETE!!!
    return (minithread_t)0;
}

int minithread_id() {               
    minithread_t tcb;
    if (queue_length(run_queue) == 0) return -1;			//-1 for no running thread
    
    tcb = (minithread_t) (run_queue->head->data);
    return tcb->id;
}

void minithread_stop() {
	//Thread shouldn't be able to run anymore (unless something happens)
	minithread_t tcb_old, tcb_new;
	tcb_old = NULL;
  if (queue_dequeue(run_queue, (void**) &tcb_old) == -1) return;		//Why Explicit casting needed here?
  tcb_new = (minithread_t) (run_queue->head->data);
  minithread_switch(&(tcb_old->stacktop), &(tcb_new->stacktop));
}

void minithread_start(minithread_t t) {			//Should this append or prepend?
	//minithread_t tcb_old;
	if (queue_append(run_queue, t) == -1) return;			//WAS prepend

	/*if (t->id != minithread_id()) {		//only need to context switch if t is not the currently running process
    	printf("Context switch!\n");
    	tcb_old = (minithread_t) (run_queue->head->data);		//Current running proc
   		minithread_switch(&(tcb_old->stacktop), &(t->stacktop));
   	}*/
}

void minithread_yield() {					//FIX!!
   	minithread_t tcb_old, tcb_new;
    tcb_old = NULL;
   	if (queue_dequeue(run_queue, (void**) &tcb_old) == -1) return;	//needed?
   	tcb_new = (minithread_t) (run_queue->head->data);
   	if (queue_append(run_queue, tcb_old) == -1) return;	//needed?
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
    char dummystack[2048];
    //proc_t func;

    if (run_queue == NULL) run_queue = queue_new();
    tcb = minithread_fork(mainproc, mainarg);         //creates and schedules at beginning
    //minithread_create(NULL, NULL);
    //tcb_dummy = (minithread_t) malloc(sizeof(struct minithread));		//Current running proc
   	//minithread_allocate_stack(&(tcb_dummy->stackbase), &(tcb_dummy->stacktop)); 		//allocate fresh stack
   	//minithread_switch(&(tcb_old->stacktop), &(t->stacktop));
   	printf("%i\n", run_queue->len);
   	printf("cur ID:%i  counter:%i\n", minithread_id(), thread_ctr);

   	minithread_switch((void**) &(dummystack), &(tcb->stacktop));

   	while(1) {
   		printf("SPINNING\n");
   	}
	/*
	//Scheduling logic
    while(1){
    	//printf("QUEUE LENGTH:%i\n", queue_length(run_queue));

    	if (queue_length(run_queue) == 0){			//SHOULDNT HAPPEN IF YOU ADDED TO QUEUE?
    		//queue_free(run_queue);
    		//return;
    	}
    	else if (queue_length(run_queue) >= 1){	//At least one on queue
    		tcb = (minithread_t) (run_queue->head)->data;
    		//func = tcb->func;
    		//func(tcb->arg);
   			if (queue_dequeue(run_queue, (void**) &tcb) == -1) return;	//Stop the guy who was run
    	}
    	
    	//else{	//Multiple threads!
    	//	(tcb->func)(tcb->arg);

    	//}
    }*/
    
}

/*
 * minithread.h:
 *  Definitions for minithreads.
 *
 *  Your assignment is to implement the functions defined in this file.
 *  You must use the names for types and procedures that are used here.
 *  Your procedures must take the exact arguments that are specified
 *  in the comments.  See minithread.c for prototypes.
 */

#ifndef __MINITHREAD_H__
#define __MINITHREAD_H__

#include "machineprimitives.h"


/*
 * struct minithread:
 *  This is the key data structure for the thread management package.
 *  You must define the thread control block as a struct minithread.
 */
typedef struct minithread *minithread_t;
struct minithread {
	int id;
	int dead;
	int priviliged;

	int run_level;
	int quant_left;

	stack_pointer_t stackbase;
	stack_pointer_t stacktop;

	proc_t func;
	arg_t arg;
};

/* CLOCK VARIABLES */
extern int clk_period;    		// Clock interrupt period
extern long clk_count;			// Running count of clock interrupts
// extern queue_t alarm_queue;		// Queue containing alarms (soonest deadline at head of queue)

/*
 * minithread_t
 * minithread_fork(proc_t proc, arg_t arg)
 *  Create and schedule a new thread of control so
 *  that it starts executing inside proc_t with
 *  initial argument arg.
 */ 
extern minithread_t minithread_fork(proc_t proc, arg_t arg);


/*
 * minithread_t
 * minithread_create(proc_t proc, arg_t arg)
 *  Like minithread_fork, only returned thread is not scheduled
 *  for execution.
 */
extern minithread_t minithread_create(proc_t proc, arg_t arg);



/*
 * minithread_t minithread_self():
 *  Return identity (minithread_t) of caller thread.
 */
extern minithread_t minithread_self();


/*
 * int minithread_id():
 *      Return thread identifier of caller thread, for debugging.
 *
 */
extern int minithread_id();


/*
 * minithread_stop()
 *  Block the calling thread.
 */
extern void minithread_stop();

/*
 * minithread_start(minithread_t t)
 *  Make t runnable.
 */
extern void minithread_start(minithread_t t);

/*
 * minithread_yield()
 *  Forces the caller to relinquish the processor and be put to the end of
 *  the ready queue.  Allows another thread to run.
 */
extern void minithread_yield();

/*
 * minithread_system_initialize(proc_t mainproc, arg_t mainarg)
 *  Initialize the system to run the first minithread at
 *  mainproc(mainarg).  This procedure should be called from your
 *  main program with the callback procedure and argument specified
 *  as arguments.
 */
extern void minithread_system_initialize(proc_t mainproc, arg_t mainarg);


/*
 * minithread_sleep_with_timeout(int delay)
 *      Put the current thread to sleep for [delay] milliseconds
 */
extern void minithread_sleep_with_timeout(int delay);


/*
 * Selects next ready process.  Acts as the scheduler, but does not deallocate dead threads
 */
extern void minithread_next(minithread_t self);

extern void minithread_deallocate(minithread_t thread);

/*
This function marks the calling thread as "dead" or done executing. However, its stack space cannot
be freed by itself. The scheduler frees the function's stack space in .... (TBD)

This function is better known as the "final_proc" called in minithread_initialize_stack.

*/
extern int minithread_exit(arg_t arg);


extern int minithread_wake(minithread_t thread);


#endif /*__MINITHREAD_H__*/


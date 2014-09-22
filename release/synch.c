#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include "defs.h"
#include "synch.h"
#include "queue.h"
#include "minithread.h"

/*
 *      You must implement the procedures and types defined in this interface.
 */


/* NOTE: struct semaphore defined in synch.h */

/*
 * semaphore_t semaphore_create()
 *      Allocate a new semaphore.
 */
semaphore_t semaphore_create() {
    return (semaphore_t) malloc(struct semaphore);    //Is the following a cast: return (semaphore_t)0;
}


/*
 * semaphore_destroy(semaphore_t sem);
 *      Deallocate a semaphore.
 */
void semaphore_destroy(semaphore_t sem) {		//CHECK
	//Cases: queue is not empty when this is called: BAD!	... what else could go wrong?
	free(sem);
}


/*
 * semaphore_initialize(semaphore_t sem, int cnt)
 *      initialize the semaphore data structure pointed at by
 *      sem with an initial value cnt.
 */
void semaphore_initialize(semaphore_t sem, int cnt) {
	if (sem == NULL){
		printf("Tried to initialize null semaphore.\n");
		return;
	}
	sem->count = cnt;

	//ARE THESE INITIALIZATIONS ALSO DESIRED HERE? (Currently semaphore_create does no initializations)
	sem->lock = 0;
	sem->wait_queue = queue_new;
}


/*
 * semaphore_P(semaphore_t sem)
 *      P on the semaphore.
 */
void semaphore_P(semaphore_t sem) {		//"Wait"
	while(atomic_test_and_set(sem->lock) == 1); //Atomically check lock status and leave locked
	if (--(s->count) < 0) {		//Resource unavailable
		
		//append calling process to wait queue
		queue_append(sem->wait_queue, (void*) minithread_self());

		//Release lock
		s->lock = 0;
		
		//context switch to next guy on run_queue
		minithread_yield();
	}
	else {						//Resource obtained, continue operation
		s->lock = 0;
	} 
}

/*
 * semaphore_V(semaphore_t sem)
 *      V on the semaphore.
 */
void semaphore_V(semaphore_t sem) {		//"Signal"
	while(atomic_test_and_set(sem->lock) == 1); //Atomically check lock status and leave locked
	if (++(s->count) <= 0) {		//COMMENT?
		queue_dequeue()
		//take me off wait queue: I will be at head with FIFO semaphore request
		//make runnable
	}
	s->lock = 0;

}

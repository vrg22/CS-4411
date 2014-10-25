#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include "synch.h"
#include "interrupts.h"

/*
 *      You must implement the procedures and types defined in this interface.
 */


/* NOTE: struct semaphore defined in synch.h */


/*
 * semaphore_t semaphore_create()
 *      Allocate a new semaphore.
 */
semaphore_t semaphore_create() {
	semaphore_t sem = (semaphore_t) malloc(sizeof(struct semaphore));    //Is the following a cast: return (semaphore_t)0;
	if (sem == NULL) { // malloc() failed
        fprintf(stderr, "ERROR: semaphore_create() failed to malloc new semaphore_t\n");
        return NULL;
    }

    return sem;
}


/*
 * semaphore_destroy(semaphore_t sem);
 *      Deallocate a semaphore.
 */
void semaphore_destroy(semaphore_t sem) {
	if (sem == NULL) {
        fprintf(stderr, "ERROR: semaphore_destroy() received NULL argument semaphore\n");
        return;
    }

	queue_free(sem->wait_queue);
	free(sem);
}


/*
 * semaphore_initialize(semaphore_t sem, int cnt)
 *      initialize the semaphore data structure pointed at by
 *      sem with an initial value cnt.
 */
void semaphore_initialize(semaphore_t sem, int cnt) {
	if (sem == NULL) {
        fprintf(stderr, "ERROR: semaphore_initialize() received NULL argument semaphore\n");
        return;
    }

	sem->count = cnt;
	sem->lock = 0;
	sem->wait_queue = queue_new();
}


/*
 * semaphore_P(semaphore_t sem)
 *      P on the semaphore.
 */
void semaphore_P(semaphore_t sem) {
	minithread_t tcb;

	interrupt_level_t old_ilevel = set_interrupt_level((interrupt_level_t) DISABLED); // Disable interrupts

	while (atomic_test_and_set(&sem->lock) == 1); // Atomically check lock status and exit loop with lock locked
	if (--(sem->count) < 0) { // Resource unavailable
		tcb = minithread_self();
		queue_append(sem->wait_queue, (void*) tcb); // Move calling process from run_queue to semaphore wait_queue
		sem->lock = 0; // Release lock
		minithread_stop(); // Context switch to next thread on run_queue
	} else { // Resource obtained; continue
		sem->lock = 0; // Release lock
	}

	set_interrupt_level(old_ilevel); // Enable interrupts
}


/*
 * semaphore_V(semaphore_t sem)
 *      V on the semaphore.
 */
void semaphore_V(semaphore_t sem) {
	minithread_t tcb;

	interrupt_level_t old_ilevel = set_interrupt_level((interrupt_level_t) DISABLED); // Disable interrupts

	while (atomic_test_and_set(&sem->lock) == 1); // Atomically check lock status and exit loop with lock locked
	if (++(sem->count) <= 0) { // Other thread(s) still waiting for resource
		queue_dequeue(sem->wait_queue, (void**) &tcb); // Remove current thread from semaphore wait_queue
		minithread_start(tcb); // Add current thread to run_queue
	}
	sem->lock = 0; // Release lock

	set_interrupt_level(old_ilevel); // Enable interrupts
}

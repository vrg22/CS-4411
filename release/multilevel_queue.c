/*
 * Multilevel queue manipulation functions  
 */
#include "multilevel_queue.h"
#include <stdlib.h>
#include <stdio.h>

/*
 * Returns an empty multilevel queue with number_of_levels levels. On error should return NULL.
 */
multilevel_queue_t multilevel_queue_new(int number_of_levels) {
	multilevel_queue_t ml_queue;
	queue_t queue;
	int i;

	ml_queue = (multilevel_queue_t) malloc(sizeof(struct multilevel_queue));	// Malloc the multilevel queue
	if (ml_queue == NULL) return NULL;	// malloc failure

	ml_queue->num_levels = number_of_levels;
	//Create each level's queue
	for (i = 0; i < number_of_levels; i++) {
		(ml_queue->levels)[i] = queue_new();
		if ((ml_queue->levels)[i] == NULL) return NULL;	// malloc failure
	}

	return ml_queue;
}


/*
 * Appends an void* to the multilevel queue at the specified level. Return 0 (success) or -1 (failure).
 */
int multilevel_queue_enqueue(multilevel_queue_t queue, int level, void* item) {
	queue_t q;
	q = (queue->levels)[level];
	return queue_append(q, item);
}

/*
 * Dequeue and return the first void* from the multilevel queue starting at the specified level. 
 * Levels wrap around so as long as there is something in the multilevel queue an item should be returned.
 * Return the level that the item was located on and that item if the multilevel queue is nonempty,
 * or -1 (failure) and NULL if queue is empty.
 */
int multilevel_queue_dequeue(multilevel_queue_t queue, int level, void** item) {
	queue_t q;
	int i, j, n, rslt;
	n = queue->num_levels;
	i = level;
	j = 0;

	while (j < n){	//check only n levels
		q = (queue->levels)[i];
		rslt = queue_dequeue(q, item);
		if (rslt == 0) return i;		// Success!

		j++;				// loop increment
		i = (i + 1) % n;	// i points to next possible level
	}

	// Multi-level queue was empty
	*item = NULL;
	return -1;
}

/* 
 * Free the queue and return 0 (success) or -1 (failure). Do not free the queue nodes; this is
 * the responsibility of the programmer.
 */
int multilevel_queue_free(multilevel_queue_t queue) {			//CHECK!
	//Assume programmer will free up each level's queue
	free (queue);
}

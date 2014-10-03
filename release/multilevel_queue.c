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
	int i;

	ml_queue = (multilevel_queue_t) malloc(sizeof(struct multilevel_queue));	// Malloc the multilevel queue
	if (ml_queue == NULL) return NULL;	// malloc failure

	ml_queue->num_levels = number_of_levels;
	ml_queue->levels = malloc(sizeof(queue_t) * number_of_levels);		//queue_t array declaration

	//Create each level's queue
	for (i = 0; i < number_of_levels; i++) {
		(ml_queue->levels)[i] = queue_new();
		if ((ml_queue->levels)[i] == NULL) {
			printf("malloc failed!\n");
			return NULL;	// malloc failure
		}
	}
	return ml_queue;
}


/*
 * Appends an void* to the multilevel queue at the specified level. Return 0 (success) or -1 (failure).
 */
int multilevel_queue_enqueue(multilevel_queue_t queue, int level, void* item) {
	queue_t q; 
	int rslt;

	q = (queue->levels)[level];
	rslt = queue_append(q, item);

	return rslt;
}

/*
 * Dequeue and return the first void* from the multilevel queue starting at the specified level. 
 * Levels wrap around so as long as there is something in the multilevel queue an item should be returned.
 * Return the level that the item was located on and that item if the multilevel queue is nonempty,
 * or -1 (failure) and NULL if queue is empty.
 */
int multilevel_queue_dequeue(multilevel_queue_t queue, int level, void** item) {
	queue_t q;
	int i, n;

	n = queue->num_levels;
	i = 0;

	while (i < n){	//check only n levels
		q = (queue->levels)[level];

		if (queue_dequeue(q, item) == 0){
			return level;		// Success!
		}

		i++;				// loop increment
		level = (level + 1) % n;	// i points to next possible level
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
	if (queue == NULL){
		return -1;
	}

	free (queue);
	return 0;
}

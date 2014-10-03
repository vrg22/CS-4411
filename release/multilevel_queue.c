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
	//queue_t lvls[number_of_levels];			//queue_t q;
	int i;

	ml_queue = (multilevel_queue_t) malloc(sizeof(struct multilevel_queue));	// Malloc the multilevel queue
	if (ml_queue == NULL) return NULL;	// malloc failure

	ml_queue->num_levels = number_of_levels;
	ml_queue->levels = malloc(sizeof(queue_t) * number_of_levels);
	/*&lvls[0]*/;		//queue_t array declaration
										//queue_t (ml_queue->levels)[ml_queue->num_levels];
	//Create each level's queue
	for (i = 0; i < number_of_levels; i++) {
		(ml_queue->levels)[i] = queue_new();
		if ((ml_queue->levels)[i] == NULL) {
			printf("malloc failed!\n");
			return NULL;	// malloc failure
		}
		printf("level: %i, len: %i\n", i, queue_length((ml_queue->levels)[i]));

		// if (i == 0){
		// 	q = (ml_queue->levels)[i];
		// 	printf("Ptr of the actual queue: %p\n", q);
		// }
	}
	printf("Levels Queue Ptr: %p\n\n", ml_queue->levels);
	return ml_queue;
}


/*
 * Appends an void* to the multilevel queue at the specified level. Return 0 (success) or -1 (failure).
 */
int multilevel_queue_enqueue(multilevel_queue_t queue, int level, void* item) {
	queue_t q; int rslt;
	// int i = 6;

	q = (queue->levels)[level];
	printf("Levels Queue Ptr: %p\n", queue->levels);
	// printf("Item Queue Ptr: %p\n", item);
	printf("Ptr of the actual queue: %p\n", q);
	
	printf("level: %i, len: %i\n", level, queue_length((queue->levels)[level]));
	// if (q != NULL){
	// 	printf("level: %i, len: %i\n", level, queue_length(q));
	// }
	
	rslt = queue_append(q, item);
	printf("level: %i, len: %i\n", level, queue_length((queue->levels)[level]));

	q = (queue->levels)[level];
	printf("Ptr of the actual queue: %p\n", q);

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
	// int rslt;
	int i, n;

	// rslt = queue_dequeue(q, item);
	// printf("Got here\n");
	// return rslt;

	// printf("in dequeue\n");
	n = queue->num_levels;
	i = 0;

	while (i < n){	//check only n levels
		printf("Levels Queue Ptr: %p\n", queue->levels);
		printf("Level: %i\n", level);
		q = (queue->levels)[level];
		// printf("Ptr of the actual queue: %p\n", q);

		if (queue_dequeue(q, item) == 0){
			printf("Successful dequeue\n");
			return level;		// Success!
		}
		else {
			printf("Did not find at level: %i\n", level);
		}
		printf("in loop\n");
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

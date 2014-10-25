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

	// Allocate space for new multilevel queue
	ml_queue = (multilevel_queue_t) malloc(sizeof(struct multilevel_queue));
	if (ml_queue == NULL) { // malloc() failed
		fprintf(stderr, "ERROR: multilevel_queue_new() failed to malloc new multilevel queue\n");
		return NULL;
	}

	ml_queue->num_levels = number_of_levels;
	ml_queue->length = 0;

	// Allocate space for array of levels
	ml_queue->levels = malloc(sizeof(queue_t) * number_of_levels);
	if (ml_queue->levels == NULL) { // malloc() failed
		fprintf(stderr, "ERROR: multilevel_queue_new() failed to malloc multilevel queue array of levels\n");
		return NULL;
	}

	// Create queue for each level
	for (i = 0; i < number_of_levels; i++) {
		(ml_queue->levels)[i] = queue_new();
		if ((ml_queue->levels)[i] == NULL) { // malloc() failed
			fprintf(stderr, "ERROR: multilevel_queue_new() failed to malloc multilevel queue level %d\n", i);
			return NULL;
		}
	}

	return ml_queue;
}


/*
 * Appends an void* to the multilevel queue at the specified level. Return 0 (success) or -1 (failure).
 */
int multilevel_queue_enqueue(multilevel_queue_t queue, int level, void* item) {
	queue_t q;

	// Check for argument errors
	if (queue == NULL) {
		fprintf(stderr, "ERROR: multilevel_queue_enqueue() received NULL argument queue\n");
		return -1;
	}
	if (level < 0 || level >= queue->num_levels) {
		fprintf(stderr, "ERROR: multilevel_queue_enqueue() received invalid queue level\n");
		return -1;
	}

	q = (queue->levels)[level];

	if (queue_append(q, item) == 0) {
		(queue->length)++;
		return 0;
	}
	
	return -1;
}

/*
 * Dequeue and return the first void* from the multilevel queue starting at the specified level. 
 * Levels wrap around so as long as there is something in the multilevel queue an item should be returned.
 * Return the level that the item was located on and that item if the multilevel queue is nonempty,
 * or -1 (failure) and NULL if queue is empty.
 */
int multilevel_queue_dequeue(multilevel_queue_t queue, int level, void** item) {
	queue_t q;
	int counter = queue->num_levels;

	// Check for argument errors
	if (queue == NULL) {
		fprintf(stderr, "ERROR: multilevel_queue_dequeue() received NULL argument queue\n");
		return -1;
	}
	if (level < 0 || level >= queue->num_levels) {
		fprintf(stderr, "ERROR: multilevel_queue_dequeue() received invalid queue level\n");
		return -1;
	}

	while (counter > 0) {	// Check only levels equal to and lower than the specified level
		q = (queue->levels)[level];

		if (queue_dequeue(q, item) == 0) { // Valid element found; terminate search
			(queue->length)--;
			return level;
		}

		level = (level + 1) % (queue->num_levels);
		counter--;
	}

	// Multi-level queue was empty
	*item = NULL;
	return -1;
}

/* 
 * Free the queue and return 0 (success) or -1 (failure). Do not free the queue nodes; this is
 * the responsibility of the programmer.
 */
int multilevel_queue_free(multilevel_queue_t queue) {
	int i, result;

	// Check for argument errors
	if (queue == NULL) {
		fprintf(stderr, "ERROR: multilevel_queue_free() received NULL argument queue\n");
		return -1;
	}

	// Free each level's queue
	result = 0;
	for (i = 0; i < queue->num_levels; i++) {
		result += queue_free((queue->levels)[i]);
	}

	if (result < 0) {
		result = -1;
	} else if (result > 0) {
		fprintf(stderr, "ERROR: multilevel_queue_free() calculated positive result (check queue_free() implementation)\n");
	}

	free(queue);
	return result;
}

int multilevel_queue_length(multilevel_queue_t queue) {
	// Check for argument errors
	if (queue == NULL) {
		fprintf(stderr, "ERROR: multilevel_queue_length() received NULL argument queue\n");
		return -1;
	}

	return queue->length;
}

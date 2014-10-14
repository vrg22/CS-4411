/*
 * Generic queue implementation.
 *
 */
#include "queue.h"
#include <stdlib.h>
#include <stdio.h>

/*
 * Return an empty queue.
 */
queue_t queue_new() {
    queue_t queue = malloc(sizeof(struct queue));
    if (queue == NULL) { // malloc() failed
        printf("ERROR: queue_new() failed to malloc new queue_t\n");
        return NULL;
    }
    
    queue->len = 0;
    return queue;
}

/*
 * Prepend a void* to a queue (both specifed as parameters).  Return
 * 0 (success) or -1 (failure).
 */
int queue_prepend(queue_t queue, void* item) {
    elem_q* elem;

    // Check for argument errors
    if (queue == NULL) {
        printf("ERROR: queue_prepend() received NULL argument queue\n");
        return -1;
    }

    // Allocate new element wrapper
    elem = (elem_q*) malloc(sizeof(elem_q));
    if (elem == NULL) { // malloc() failed
        printf("ERROR: queue_prepend() failed to malloc new elem_q\n");
        return -1;
    }

    // Initialize elem data and pointers
    elem->data = item;
    elem->next = (queue->len == 0) ? elem : queue->head;
    elem->prev = (queue->len == 0) ? elem : queue->tail;

    // Set head and tail pointers
    queue->head = elem;
    queue->tail = (queue->len == 0) ? elem : queue->tail;

    // Set old head and tail next and prev pointers
    queue->head->next->prev = queue->head;
    queue->tail->next = queue->head;

    // Update queue length
    (queue->len)++;

    return 0;
}

/*
 * Append a void* to a queue (both specifed as parameters). Return
 * 0 (success) or -1 (failure).
 */
int queue_append(queue_t queue, void* item) {
    elem_q* elem;
    
    // Check for argument errors
    if (queue == NULL) {
        printf("ERROR: queue_append() received NULL argument queue\n");
        return -1;
    }

    // Allocate new element wrapper
    elem = (elem_q*) malloc(sizeof(elem_q));
    if (elem == NULL) { // malloc() failed
        printf("ERROR: queue_append() failed to malloc new elem_q\n");
        return -1;
    }

    // Initialize elem data and pointers
    elem->data = item;
    elem->next = (queue->len == 0) ? elem : queue->head;
    elem->prev = (queue->len == 0) ? elem : queue->tail;

    // Set head and tail pointers
    queue->head = (queue->len == 0) ? elem : queue->head;
    queue->tail = elem;

    // Set old head and tail next and prev pointers
    queue->head->prev = queue->tail;
    queue->tail->prev->next = queue->tail;

    // Update queue length
    (queue->len)++;

    return 0;
}

/*
 * Dequeue and return the first void* from the queue or NULL if queue
 * is empty.  Return 0 (success) or -1 (failure).
 */
int queue_dequeue(queue_t queue, void** item) {
	elem_q* ptr;

    // Check for argument errors
    if (queue == NULL) {
        printf("ERROR: queue_dequeue() received NULL argument queue\n");
        return -1;
    }

    if (queue->len == 0) { // Empty queue
		*item = NULL;
		return -1;
	}

    // Identify head
    ptr = queue->head;
    *item = ptr->data;

    // Fix queue pointers
    if (queue->len == 1) { // Resulting queue is empty
	    queue->head = NULL;
	    queue->tail = NULL;
    } else {
	    queue->head = queue->head->next;
	    free(ptr);
	    queue->tail->next = queue->head;
	    queue->head->prev = queue->tail;
	}

    (queue->len)--;

    return 0;
}


/*
 * Iterate the function parameter over each element in the queue.  The
 * additional void* argument is passed to the function as its first
 * argument and the queue element is the second.  Return 0 (success)
 * or -1 (failure).
 */
int queue_iterate(queue_t queue, func_t f, void* item) {
    elem_q* iter;
    
    // Check for argument errors
    if (queue == NULL) {
        printf("ERROR: queue_iterate() received NULL argument queue\n");
        return -1;
    }

    if (queue->len == 0) { // Empty queue; nothing to do
        return 0;
    }

    // Iterate from queue's head to tail
    iter = queue->head;
    while (iter->next != queue->head) {
        f(item, iter->data); // Apply f to element
        iter = iter->next;
    }
    
    // Run f on final element
    f(item, iter->data);

    return 0;
}


/*
 * Free the queue and return 0 (success) or -1 (failure).
 */
int queue_free (queue_t queue) {
    elem_q* ptr;

    // Check for argument errors
    if (queue == NULL) {
        printf("ERROR: queue_free() received NULL argument queue\n");
        return -1;
    }

    // Free queue elements
    while (queue->head != NULL){
        free(queue->head->data); // Free element data

        ptr = queue->head;
        queue->head = queue->head->next;

        free(ptr); // Free element
        ptr = NULL; // Set ptr to NULL to avoid dangling pointer when iteration reaches queue->tail and attempts to access the freed head element
    }

    free(queue);
    return 0;
}


/*
 * Return the number of items in the queue. Return -1 if passed in NULL queue pointer.
 */
int queue_length(queue_t queue) {
	// Check for argument errors
    if (queue == NULL) {
        printf("ERROR: queue_length() received NULL argument queue\n");
        return -1;
    }

    return queue->len;
}


/*
 * Delete the specified item from the given queue.
 * Return 0 on success. Return -1 on error.
 */
int queue_delete(queue_t queue, void* item) {
	elem_q* iter;

	// Check for argument errors
    if (queue == NULL) {
        printf("ERROR: queue_delete() received NULL argument queue\n");
        return -1;
    }
    
    if (queue->len == 0) { // Empty queue (nothing to delete)
		return -1;
	}

    // Find item in queue
    iter = queue->head;
    while ((iter->data != item) && (iter != queue->tail)){
    	iter = iter->next;
    }

    if (iter->data == item) { // Found item
    	// Update references of adjacent elements
    	(iter->prev)->next = iter->next;
    	(iter->next)->prev = iter->prev;

    	// Update queue head/tail pointers
    	if (queue->len > 1) { // Resulting queue is not empty
    		if (iter == queue->head)  // Correct head pointer
    			queue->head = iter->next;
    		if (iter == queue->tail) // Correct tail pointer
   				queue->tail = iter->prev;
   		} else { // Resulting queue is empty
   			queue->head = NULL;
   			queue->tail = NULL;
   		}
   		(queue->len)--;

   		// Free element wrapper
   		iter->next = NULL;
   		iter->prev = NULL;
   		iter->data = NULL;
   		free(iter);

    	return 0;
    } else { // item not found
 		return -1;
	}
}
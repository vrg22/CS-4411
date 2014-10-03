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
    if (queue == NULL) return NULL;   // malloc failure
    
    queue->len = 0;
    return queue;
}

/*
 * Prepend a void* to a queue (both specifed as parameters).  Return
 * 0 (success) or -1 (failure).
 */
int queue_prepend(queue_t queue, void* item) {
    elem_q* elem;
    if (queue == NULL) return -1;   // Failure: queue null

    elem = (elem_q*) malloc(sizeof(elem_q));
    if (elem == NULL) {
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
    if (queue == NULL) return -1;   // Failure: queue null

    elem = (elem_q*) malloc(sizeof(elem_q));
    if (elem == NULL) {
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
	if (queue == NULL) return -1;   // Failure: queue null

    if (queue->len == 0) {      //Empty queue
		*item = NULL;
		return -1;              // Failure: nothing to dequeue
	}

    // Identify head
    ptr = queue->head;
    *item = ptr->data;

    if (queue->len == 1) {
	    // Update Queue
	    queue->head = NULL;
	    queue->tail = NULL;
    } else {
        // Update head
	    queue->head = queue->head->next;
	    free (ptr);

	    // Correct new Head/Tail
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
    if (queue == NULL) return -1;   // Failure: queue null

    if (queue->len == 0)    // Nothing to do
        return 0;

    iter = queue->head;
    while(iter->next != queue->head){
        // Run Function
        f(item, iter->data);
        iter = iter->next;
    }
    
    // Run Function on final element
    f (item, iter->data);

    return 0;
}


/*
 * Free the queue and return 0 (success) or -1 (failure).
 */
int queue_free (queue_t queue) {
    elem_q* ptr;
    if (queue == NULL) return -1;   // Failure: queue null

    //Free queue elements
    while(queue->head != NULL){
        free (queue->head->data);  //free associated data
        queue->head->data = NULL;

        ptr = queue->head;
        queue->head = queue->head->next;

        free (ptr);               //Free element
        ptr = NULL;
    }

    free (queue);
    queue = NULL;   // Make queue a NULL pointer
    return 0;
}


/*
 * Return the number of items in the queue. Return -1 if passed in NULL queue pointer.
 */
int queue_length(queue_t queue) {
	if (queue == NULL) return -1;   // Failure: queue null

    return queue->len;
}


/*
 * Delete the specified item from the given queue.
 * Return 0 on success. Return -1 on error.
 */
int queue_delete(queue_t queue, void* item) {
	elem_q* iter;
	if (queue == NULL) return -1;   // Failure: queue null
    
    //Nothing to delete
    if (queue->len == 0) {
		return -1;            //Didn't delete anything
	}

    iter = queue->head;
    while ((iter->data != item) && (iter != queue->tail)){
    	iter = iter->next;
    }

    //Found item
    if (iter->data == item) {
    	//Update references of adjacent elts
    	(iter->prev)->next = iter->next;
    	(iter->next)->prev = iter->prev;

    	//Update queue ptrs to head/tail
    	if(queue->len > 1) {	//More than one elt
    		if(iter == queue->head)	//was head but NOT tail
    			queue->head = iter->next;
    		if(iter == queue->tail)	//was tail but NOT head
   				queue->tail = iter->prev;
   		}
   		else{
   			queue->head = NULL;
   			queue->tail = NULL;
   		}
   		(queue->len)--;

   		//Free elt
   		iter->next = NULL;
   		iter->prev = NULL;
   		iter->data = NULL;	//Is this WRONG / is item a distinct copy of this pointer?
   		free (iter);

    	return 0;
    }

    //Item not found
	else{
 		return -1;	//Nothing deleted
	}
}
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
queue_t
queue_new() {
    queue_t queue = malloc(sizeof(struct queue));
    if (queue == NULL) return NULL;
    
    queue->len = 0;
    return queue;
}

/*
 * Prepend a void* to a queue (both specifed as parameters).  Return
 * 0 (success) or -1 (failure).
 */
int
queue_prepend(queue_t queue, void* item) {
    elem_q* elt = (elem_q*) malloc(sizeof(elem_q));
    if (elt == NULL) {
        return -1;
    }
    elt->data = item;
    elt->next = (queue->len == 0) ? elt : queue->head;
    elt->prev = (queue->len == 0) ? elt : queue->tail;

    //make sure head/tail are defined
    queue->head = elt;
    queue->tail = (queue->len == 0) ? elt : queue->tail;
    (queue->len)++;

    //Need to adjust current head/tail pointers
    queue->head->prev = elt;
    queue->tail->next = elt;

    return 0;
}

/*
 * Append a void* to a queue (both specifed as parameters). Return
 * 0 (success) or -1 (failure).
 */
int
queue_append(queue_t queue, void* item) {
    elem_q* elt =  (elem_q*) malloc(sizeof(elem_q));
    if (elt == NULL) {
        return -1;
    }
    elt->data = item;
    elt->next = (queue->len == 0) ? elt : queue->head;
    elt->prev = (queue->len == 0) ? elt : queue->tail;

    //make sure head/tail are defined
    queue->head = (queue->len == 0) ? elt : queue->head;
    queue->tail = elt;
    (queue->len)++;

    //Need to adjust current head/tail's pointers
    queue->head->prev = elt;
    queue->tail->next = elt;

    return 0;
}

/*
 * Dequeue and return the first void* from the queue or NULL if queue
 * is empty.  Return 0 (success) or -1 (failure).
 */
int
queue_dequeue(queue_t queue, void** item) {
	elem_q* ptr;

	if (queue->len == 0) {
		*item = NULL;
		return -1;

	} else if (queue->len == 1) {
		//Identify head
		ptr = (queue->head);
	    *item = ptr->data;

	    //Update Queue
	    queue->head = NULL;
	    queue->tail = NULL;
	    (queue->len)--;
		
	    //Free element
		ptr->data = NULL;		//Is a pointer; good practice to set to NULL, but Necessary?
	    ptr->next = NULL;
	    ptr->prev = NULL;
	    free (ptr);

	    return 0;

    } else {
    	//Identify head
	    ptr = (queue->head);	
	    *item = ptr->data;

	    //Update head
	    queue->head = (queue->head)->next;
	    (queue->len)--;
	    
	    //Correct new Head/Tail
	    (queue->tail)->next = (queue->head);
	    (queue->head)->prev = (queue->tail);

	    //Free element
	    ptr->data = NULL;
	    ptr->next = NULL;
	    ptr->prev = NULL;
	    free (ptr);

	    return 0;
	}
}



/*
 * Iterate the function parameter over each element in the queue.  The
 * additional void* argument is passed to the function as its first
 * argument and the queue element is the second.  Return 0 (success)
 * or -1 (failure).
 */
int
queue_iterate(queue_t queue, func_t f, void* item) {
    elem_q* iter;

    if (queue == NULL)      //Failure: Null pointer
        return -1;

    if (queue->len == 0)    //nothing to do
        return 0;

    iter = queue->head;
    while(iter->next != queue->head){
        //Run Function
        f(item, iter->data);
        iter = iter->next;
    }
    
    //Run Function on final element
    f(item, iter->data);

    return 0;
}

/*
 * Free the queue and return 0 (success) or -1 (failure).
 */
int
queue_free (queue_t queue) {                //MAY NEED WHILE LOOP TO FREE ALL THE ELEMENTS
    if (queue == NULL) {
    	return -1;
    }
    free (queue);
    queue = NULL;   //Make queue a NULL pointer     //Correct?
    return 0;
}

/*
 * Return the number of items in the queue.
 */
int
queue_length(queue_t queue) {
	if (queue == NULL) {	//Correct?
    	return -1;
    }
    return queue->len;
}


/*
 * Delete the specified item from the given queue.
 * Return 0 on success. Return -1 on error.
 */
int
queue_delete(queue_t queue, void* item) {
	elem_q* iter;

	//Nothing to delete
	if (queue == NULL || queue->len == 0) {
		return -1;
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
 		return -1;	
	}
}
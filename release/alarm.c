#include <stdlib.h>
#include <stdio.h>

#include "alarm.h"

queue_t alarm_queue = NULL; // Queue containing alarms (soonest deadline at head of queue)

/* CLOCK VARIABLES */
long clk_period = 100 * MILLISECOND; // Clock interrupt period
long clk_count = 0;                 // Running count of clock interrupts


/* see alarm.h */
alarm_id register_alarm(int delay, alarm_handler_t alarm, void *arg) {
	elem_q* iter;
	elem_q* elem;
	alarm_t new_alarm;
	int not_added = 1;

	/* Initialize new alarm */
    new_alarm = (alarm_t) malloc(sizeof(struct alarm));
    if (new_alarm == NULL) { // malloc() failed
        fprintf(stderr, "ERROR: register_alarm() failed to malloc new alarm_t\n");
        return NULL;
    }
    new_alarm->deadline = ((unsigned long long) clk_count) * clk_period + ((unsigned long long) delay) * MILLISECOND;
    new_alarm->func = alarm;
    new_alarm->arg = arg;
    new_alarm->executed = 0;

    /* Initialize new queue element */
    elem = (elem_q*) malloc(sizeof(elem_q));
    if (elem == NULL) { // malloc() failed
        fprintf(stderr, "ERROR: register_alarm() failed to malloc new elem_q\n");
        return NULL;
    }
    elem->data = new_alarm;

    /* Add new alarm's wrapper element to alarm queue */
    if (alarm_queue == NULL) { // Ensure alarm_queue has been initialized
        alarm_queue = queue_new();
    }
    if (queue_length(alarm_queue) == 0) { // Nothing in alarm_queue; simply add new alarm
    	queue_append(alarm_queue, new_alarm);
    } else {
    	iter = alarm_queue->head;

    	while (iter->next != alarm_queue->head && not_added) {
    		if (((alarm_t) (iter->data))->deadline > new_alarm->deadline) { // Found the position to place new alarm in alarm_queue
    			elem->next = iter;
    			elem->prev = iter->prev;
    			iter->prev->next = elem;
    			iter->prev = elem;
    			if (iter == alarm_queue->head)
    				alarm_queue->head = elem;
    			not_added = 0;
    		}
    		iter = iter->next;
    	}

    	// New alarm has latest deadline; insert at end of queue (iter now points to alarm_queue->TAIL       NOT  head)
    	if (not_added) {
            queue_append(alarm_queue, new_alarm);
            // elem->data = NULL;
            // free(elem);


            /* Below is probably wrong */
            // elem->next = iter->next;        //Head of queue
            // elem->prev = iter;
            // elem->prev->next = elem;
            // elem->next->prev = elem; 
            // //iter->next = elem;
            // //iter->next = elem;
            // //iter->prev = (iter->prev == iter) ? elem : iter->prev;
            // alarm_queue->tail = elem;

            /*Below is wrong...*/
    		// elem->next = iter;
    		// elem->prev = iter->prev;
    		// iter->prev->next = elem;
    		// iter->prev = elem;
   			// alarm_queue->tail = elem;
    		not_added = 0;
    	}
    }

    return (alarm_id) new_alarm;
}

/* see alarm.h */
int deregister_alarm(alarm_id alarm) {
    // void* temp;

	int executed = ((alarm_t) alarm)->executed;

    if (queue_delete(alarm_queue, (alarm_t) alarm) < 0) {
        fprintf(stderr, "ERROR: deregister_alarm() failed to delete alarm\n");
    }

    // if (queue_dequeue(alarm_queue, &temp) < 0) {
    //     fprintf(stderr, "ERROR: deregister_alarm() failed to DEQUEUE alarm\n");
    // }

    return executed;
}

/*
** vim: ts=4 sw=4 et cindent
*/
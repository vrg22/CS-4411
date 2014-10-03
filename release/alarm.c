#include <stdlib.h>
#include <stdio.h>

#include "interrupts.h"
#include "alarm.h"
#include "minithread.h"
#include "queue.h"

queue_t alarm_queue = NULL;        // Queue containing alarms (soonest deadline at head of queue)

/* see alarm.h */
alarm_id register_alarm(int delay, alarm_handler_t alarm, void *arg) {
	elem_q* iter;
	elem_q* elem;
	alarm_t new_alarm;
	int not_added = 1;

	if (alarm_queue == NULL) {
		alarm_queue = queue_new();
	}

	/* Initialize new alarm */
    new_alarm = (alarm_t) malloc(sizeof(struct alarm));
    if (new_alarm == NULL) return NULL;
    new_alarm->deadline = clk_count * clk_period + delay * clk_period / MILLISECOND;
    new_alarm->thread = minithread_self();
    new_alarm->executed = 0;

    /* Initialize new queue element */
    elem = (elem_q*) malloc(sizeof(elem_q));
    if (elem == NULL) return NULL;
    elem->data = new_alarm;

    /* Add new alarm's wrapper element to alarm queue */
    if (alarm_queue == NULL) return NULL;
    if (queue_length(alarm_queue) == 0) {
    	queue_append(alarm_queue, new_alarm);
    } else {
    	iter = alarm_queue->head;

    	while (iter->next != alarm_queue->head && not_added) {
    		if (((alarm_t) (iter->data))->deadline > new_alarm->deadline) {
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

    	// Run Function on final element
    	if (not_added) {
    		elem->next = iter;
    		elem->prev = iter->prev;
    		iter->prev->next = elem;
    		iter->prev = elem;
   			alarm_queue->tail = elem;
    		not_added = 0;
    	}
    }

    return (alarm_id) new_alarm;
}

/* see alarm.h */
int deregister_alarm(alarm_id alarm) {
	int executed = ((alarm_t) alarm)->executed;
    queue_delete(alarm_queue, (alarm_t) alarm);

    return executed;
}

/*
** vim: ts=4 sw=4 et cindent
*/

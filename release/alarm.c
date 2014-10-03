#include <stdio.h>

#include "interrupts.h"
#include "alarm.h"
#include "minithread.h"
#include "queue.h"

/* see alarm.h */
alarm_id register_alarm(int delay, alarm_handler_t alarm, void *arg) {
	elem_q* iter;
    alarm_t new_alarm = (alarm_t) malloc(sizeof(struct alarm));
    if (new_alarm == NULL) return NULL;
    
    new_alarm->deadline = current_time + delay * clk_period / MILLISECOND;
    new_alarm->thread = minithread_self();

    if (alarm_queue == NULL) {
    	// YOU FUCKED UP BIG TIME, MISTER!
    }

    if (queue_length(alarm_queue) == 0) {
    	queue_append(alarm_queue, new_alarm);
    } else {
    	iter = alarm_queue->head;

    	while(iter->next != alarm_queue->head) {
    		if (((alarm_t)(iter->data))->deadline > new_alarm->deadline) {
    			
    		}
    		iter = iter->next;
    	}

    	// Run Function on final element
    	f(item, iter->data);
    }

    return (alarm_id) new_alarm;
}

/* see alarm.h */
int deregister_alarm(alarm_id alarm) {
    return 1;
}

/*
** vim: ts=4 sw=4 et cindent
*/

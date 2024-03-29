#ifndef __ALARM_H__
#define __ALARM_H__ 1

/*
 * This is the alarm interface. You should implement the functions for these
 * prototypes, though you may have to modify some other files to do so.
 */


#include "queue.h"
#include "interrupts.h"

/* An alarm_handler_t is a function that will run within the interrupt handler.
 * It must not block, and it must not perform I/O or any other long-running
 * computations.
 */
typedef void (*alarm_handler_t)(void*);
typedef void *alarm_id;

typedef struct alarm *alarm_t;
struct alarm {
	unsigned long long int deadline; // Absolute deadline in ms
	int executed;
	void* func;
	void* arg;
};

/* CLOCK VARIABLES */
extern long clk_period;    		// Clock interrupt period
extern long clk_count;			// Running count of clock interrupts

extern queue_t alarm_queue;	// Queue containing alarms (soonest deadline at head of queue)


/* register an alarm to go off in "delay" milliseconds.  Returns a handle to
 * the alarm.
 */
extern alarm_id register_alarm(int delay, alarm_handler_t func, void *arg);

/* unregister an alarm.  Returns 0 if the alarm had not been executed, 1
 * otherwise.
 */
extern int deregister_alarm(alarm_id id);

#endif

/* alarm_test.c

   Test an alarm
*/

#include <stdio.h>
#include <stdlib.h>

#include "minithread.h"
#include "alarm.h"


void functor() {
	printf("functor() running\n");
	return;
}

int thread(int* arg) {
	register_alarm(6400, (alarm_handler_t) functor, NULL);
	printf("Hello, world!\n");
	// while (1);
	// minithread_sleep_with_timeout(5000);
	return 0;
}

int main(void) {
	minithread_system_initialize(thread, NULL);
	return 0;
}

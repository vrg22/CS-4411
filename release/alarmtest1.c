/* 
 * Spawn three threads and get them to sleep.
*/

#include "minithread.h"

#include <stdio.h>
#include <stdlib.h>


int thread3(int* arg) {
  printf("Thread 3 executes and finishes.\n");

  return 0;
}

int thread2(int* arg) {
  /*minithread_t thread =*/ minithread_fork(thread3, NULL);
  printf("Thread 2 starts.\n");
	minithread_sleep_with_timeout(1000); /* ten seconds */
  printf("Thread 2 just woke up and finishes\n");

  // printf("Curr time: %llu\n", ((unsigned long long) clk_count) * clk_period);

  return 0;
}

int thread1(int* arg) {
  minithread_fork(thread2, NULL);
  printf("Thread 1 starts.\n");
	minithread_sleep_with_timeout(500); /* five seconds */
  printf("Thread 1 just woke up, and is going to sleep again.\n");
	minithread_sleep_with_timeout(1500); /* fifteen seconds */
  printf("Thread 1 just woke up and finishes\n");

  return 0;
}

int main(void) {
  minithread_system_initialize(thread1, NULL);
  return -1;
}

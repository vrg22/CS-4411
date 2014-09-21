/* test-queue.c

   Test the implementation of queue
*/

#include "queue.h"
#include <stdio.h>
#include <stdlib.h>


void
append_test(queue_t queue) {
  int  genie1, genie2; void* item;
  genie1 = 1;
  genie2 = 2;

  queue_dequeue(queue, &item);

  queue_prepend(queue, (void*) &genie1);
  queue_append(queue, (void*) &genie2);
  queue_dequeue(queue, &item);
}


int
main(void) {
  //elem_q* ptr;
  queue_t queue = malloc(sizeof(struct queue));

  queue->len = 0;
  append_test(queue);

  if (queue->len == 1)
  {
  	printf("Success!!!\n");
  }else{
  	  printf("Failure...\n");
  }

  return 0;
}

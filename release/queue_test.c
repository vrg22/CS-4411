/* queue_test.c

   Test the implementation of queue.
*/

#include "queue.h"
#include <stdio.h>
#include <stdlib.h>


void append_test(queue_t queue) {
  int  genie1, genie2, genie3, genie4, genie5;
  void* item;
  // int i=0;

  genie1 = 1;
  genie2 = 2;
  genie3 = 3;
  genie4 = 4;
  genie5 = 5;

  queue_dequeue(queue, &item);

  queue_prepend(queue, (void*) &genie2);
  queue_append(queue, (void*) &genie3);
  queue_prepend(queue, (void*) &genie1);
  queue_prepend(queue, (void*) &genie4);
  // queue_append(queue, (void*) &genie4);
  queue_append(queue, (void*) &genie5);

  // [4,1,2,3,5]
  // queue_dequeue(queue, &item); // Should be genie4
  queue_delete(queue, (void*) &genie5);

  // Print
  while(queue_dequeue(queue, &item) > -1) {
    fprintf(stderr, "%i\n", *((int*) item));
  }
}


int main(void) {
  //elem_q* ptr;
  queue_t queue = malloc(sizeof(struct queue));

  queue->len = 0;
  append_test(queue);

  // if (queue->len == 5) {
  //   printf("Success!!!\n");
  // } else {
  //   printf("Failure...\n");
  // }

  return 0;
}

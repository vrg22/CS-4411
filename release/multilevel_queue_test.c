/* multilevel_queue_test.c

   Test the implementation of multilevel queue.
*/

#include "multilevel_queue.h"
#include <stdio.h>
#include <stdlib.h>


void enqueue_test(multilevel_queue_t queue, void** item) {
  int  /*genie1,*/ genie2, genie3, genie4, genie5;

  // genie1 = 1;
  genie2 = 2;
  genie3 = 3;
  genie4 = 4;
  genie5 = 5;

  multilevel_queue_dequeue(queue, 3, item);      //CHECK that num levels valid???

  // multilevel_queue_enqueue(queue, 0, (void*) &genie1);
  multilevel_queue_enqueue(queue, 1, (void*) &genie4);
  multilevel_queue_enqueue(queue, 2, (void*) &genie3);
  multilevel_queue_enqueue(queue, 3, (void*) &genie2);
  multilevel_queue_enqueue(queue, 3, (void*) &genie5);

  multilevel_queue_dequeue(queue, 0, item); 

}


int main(void) {
  void* item;
  multilevel_queue_t queue;

  queue = multilevel_queue_new(4);
  item = NULL;
  enqueue_test(queue, &item);

  if(*((int*)(item)) == 4){
    printf("Success!!!\n");
  }else{
    printf("Failure...\n");
  }

  return 0;
}

/* test1.c

   Spawn a single thread.
*/

#include "minithread.h"

#include <stdio.h>
#include <stdlib.h>


int thread(int* arg) {
  // char buffer[100] = "adsf";
  // char* shift = "bobby" + (char)2;
  // int i = 0;

  // while(shift[i]){
  // 	printf("%c\n", shift[i]);
  // 	i++;
  // }

 	//  printf("Buffer: %s\n", buffer);
	// /* Fill in the buffer with numbers from 0 to BUFFER_SIZE-1 */
	// for (i=0; i<100; i++){
	// 	buffer[i]=(char)(i%256);
	// 	printf("Buffer char: %c\n", buffer[i]);
	// }
	// printf("Buffer: %s\n\n", buffer);

  printf("Hello, world!\n");

  return 0;
}

int main(void) {
  minithread_system_initialize(thread, NULL);
  return 0;
}

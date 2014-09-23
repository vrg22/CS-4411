/* shop.c

    Simulates the following:

    There are N employees unpacking phones.
      -each phone has unique serial number
      -phones unpacked in increasing serial order

    There are M customers, served FIFO.

*/


#include "minithread.h"
#include "synch.h"

#include <stdio.h>
#include <stdlib.h>


#define NUMPHONES                300

#define M /*customers*/          400
#define N /*employees*/          30


//Binary semaphores
semaphore_t phone;
semaphore_t line;
semaphore_t record_keeping;

int available_phones = NUMPHONES;
int unpacked_phones = 0;
int phoneID = 1;


//Unpack a phone.
void unpack_phone(){
  if (available_phones > 0){
    semaphore_P(record_keeping);
    available_phones--;
    unpacked_phones++;
    semaphore_V(record_keeping);
  }
}

//Print phone ID upon receipt
void take_phone(){
  if (unpacked_phones > 0){  //You're actually getting a phone
    //Customer prints
    printf("My Phone's ID: %i\n", phoneID++);
    unpacked_phones--;
  }
  else {
    // Dang, I didn't get a phone...
  }
}

//A new customer.
int customer(int* arg){

  //Get phone
  semaphore_P(phone);
  while (unpacked_phones == 0 && available_phones > 0) {    //No phones I can get but there are phones to be unpacked
    minithread_yield();  //give up processor
  }
  take_phone();
  semaphore_V(phone);

  return 0;
}

//A new employee
int employee(int* arg){
  while (available_phones > 0){
    unpack_phone();
    minithread_yield();
  }

  return 0;
}

//Creates the N employees & M customers.
int init_sale(int* arg) {
  int i;

  //create employees
  for(i = 0; i < N; i++){
    minithread_fork(employee, NULL);
  }

  //create customers
  for(i = 0; i < M; i++){
    minithread_fork(customer, NULL);
  }

  return 0;
}


int main(int argc, char *argv[]) {
  line = semaphore_create();
  semaphore_initialize(line, 1);
  
  phone = semaphore_create();
  semaphore_initialize(phone, 1);
  
  record_keeping = semaphore_create();
  semaphore_initialize(record_keeping, 1);

  //start
  minithread_system_initialize(init_sale, NULL);

  return -1;
}

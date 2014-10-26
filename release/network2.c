/* network test program 2

   local loopback test: spawns two threads, one of which sends a stream of
   messages and exits, the other of which calls receive the same number of
   times.

   USAGE: ./network2 <port>

   where port is the minimsg port to listen on.
*/

#include "defs.h"
#include "minithread.h"
#include "minimsg.h"
#include "synch.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define BUFFER_SIZE 256
#define MAX_COUNT 100

miniport_t port;

int
receive(int* arg) {
    char buffer[BUFFER_SIZE];
    int length;
    int i;
    miniport_t from;

    for (i=0; i<MAX_COUNT; i++) {
        length = BUFFER_SIZE;
        minimsg_receive(port, &from, buffer, &length);
        printf("%s", buffer);
        miniport_destroy(from);
    }

  return 0;
}

int
transmit(int* arg) {
    char buffer[BUFFER_SIZE];
    int length;
    int i;
    miniport_t write_port;
    network_address_t my_address;

    network_get_my_address(my_address);
    port = miniport_create_unbound(0);
    write_port = miniport_create_bound(my_address, 0);

    minithread_fork(receive, NULL);

    for (i=0; i<MAX_COUNT; i++) {
        printf("Sending packet %d.\n", i+1);
        sprintf(buffer, "Count is %d.\n", i+1);
        length = strlen(buffer) + 1;
        minimsg_send(port, write_port, buffer, length);
    }

    return 0;
}

int
main(int argc, char** argv) {
    short fromport;
    fromport = atoi(argv[1]);
    network_udp_ports(fromport,fromport); 
    minithread_system_initialize(transmit, NULL);
    return -1;
}



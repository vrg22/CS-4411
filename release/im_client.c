/* 
IM Client
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

int receive(int* arg) {
    char buffer[BUFFER_SIZE];
    int length;
    // int i;
    miniport_t from;

    port = miniport_create_unbound(100);

    // for (i=0; i<MAX_COUNT; i++) {
    while(1) {
        length = BUFFER_SIZE;
        minimsg_receive(port, &from, buffer, &length);
        printf("%s\n", buffer);
        miniport_destroy(from);
    }
    // }

  return 0;
}


int main(int argc, char** argv) {
    // short fromport;
    // fromport = atoi(argv[1]);
    // network_udp_ports(fromport,fromport); 
    minithread_system_initialize(receive, NULL);
    return -1;
}
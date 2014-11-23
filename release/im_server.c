/*
IM Server (where the message is sent from)
-specify a node
-one-line message
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

int transmit(int* arg) {
    char buffer[BUFFER_SIZE];
    int length;
    // int i;
    miniport_t write_port;
    network_address_t my_address;

    network_get_my_address(my_address);
    port = miniport_create_unbound(0);
    write_port = miniport_create_bound(my_address, 0);
    // write_port = miniport_create_bound("127.111.111.112", 0);

    // minithread_fork(receive, NULL);

    // for (i=0; i<MAX_COUNT; i++) {
        printf("Sending instant message:\n");
        sprintf(buffer, "PUT INSTANT MESSAGE HERE\n");
        length = strlen(buffer) + 1;
        minimsg_send(port, write_port, buffer, length);
    // }

    return 0;
}


int main(int argc, char** argv) {
    short fromport;
    fromport = atoi(argv[1]);
    network_udp_ports(fromport,fromport); 
    minithread_system_initialize(transmit, NULL);
    return -1;
}
/*
IM app
*/

#include "defs.h"
#include "minithread.h"
#include "minimsg.h"
#include "synch.h"

#include "read.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define BUFFER_SIZE 256
#define MAX_COUNT 100

miniport_t transmit_reply_port;
miniport_t receive_port;

int receive(int* arg) {
    char buffer[BUFFER_SIZE];
    int length;
    // int i;
    miniport_t from;

    receive_port = miniport_create_unbound(100);

    // for (i=0; i<MAX_COUNT; i++) {
    while(1) {
        length = BUFFER_SIZE;
        minimsg_receive(receive_port, &from, buffer, &length);
        printf("%s\n", buffer);
        miniport_destroy(from);
    }
    // }

    return 0;
}

int transmit(int* arg) {
    char buffer[BUFFER_SIZE];
    int length;
    // int i;
    miniport_t write_port;
    network_address_t my_address;

    network_get_my_address(my_address);
    transmit_reply_port = miniport_create_unbound(0);
    write_port = miniport_create_bound(my_address, 0);
    // write_port = miniport_create_bound("127.111.111.112", 0);

    minithread_fork(receive, NULL);

    while (1) {
        printf("Sending instant message:\n");
        
        // sprintf(buffer, "PUT INSTANT MESSAGE HERE\n");
        // length = strlen(buffer) + 1;
        length = miniterm_read(buffer, 100);
        minimsg_send(transmit_reply_port, write_port, buffer, length);
    }

    return 0;
}


void run(int* arg) {
    return;
}


int main(int argc, char** argv) {
    short fromport;
    fromport = atoi(argv[1]);
    network_udp_ports(fromport,fromport);
    minithread_system_initialize(transmit, NULL);
    return -1;
}
#ifndef __MINISOCKETS_H_
#define __MINISOCKETS_H_

/*
 *	Definitions for minisockets.
 *
 *      You should implement the functions defined in this file, using
 *      the names for types and functions defined here. Functions must take
 *      the exact arguments in the prototypes.
 *
 *      miniports and minisockets should coexist.
 */

#include "network.h"
#include "minimsg.h"

// Define minisocket port num limits
#define SERVER_MIN_PORT 0
#define SERVER_MAX_PORT 32767
#define CLIENT_MIN_PORT 32768
#define CLIENT_MAX_PORT 65536

#define NUM_SERVER_PORTS (SERVER_MAX_PORT - SERVER_MIN_PORT + 1)
#define NUM_CLIENT_PORTS (CLIENT_MAX_PORT - CLIENT_MIN_PORT + 1)

extern miniport_t* skt_ports; // Array of miniports for ports
extern semaphore_t skt_mutex; // Mutual exclusion semaphore

struct minisocket {
  int active; // 1 if socket is connected to another socket; 0 if socket is free/waiting/listening

	int local_port;  // Local port from which socket will send
  int remote_port; // Remote port to which socket will send
  network_address_t dest_address;

  semaphore_t datagrams_ready; // Allows socket to wait for messages (i.e. SYNACK, etc.)
  queue_t incoming_data; // Queue of incoming messages

  int seqnum; // Current sequence number
  int acknum; // Current ack number
};

typedef struct minisocket* minisocket_t;
typedef enum minisocket_error minisocket_error;

enum minisocket_error {
  SOCKET_NOERROR = 0,
  SOCKET_NOMOREPORTS,   /* ran out of free ports */
  SOCKET_PORTINUSE,     /* server tried to use a port that is already in use */
  SOCKET_NOSERVER,      /* client tried to connect to a port without a server */
  SOCKET_BUSY,          /* client tried to connect to a port that is in use */
  SOCKET_SENDERROR,
  SOCKET_RECEIVEERROR,
  SOCKET_INVALIDPARAMS, /* user supplied invalid parameters to the function */
  SOCKET_OUTOFMEMORY    /* function could not complete because of insufficient memory */
};


/* Initializes the minisocket layer. */
void minisocket_initialize();

/* 
 * Listen for a connection from somebody else. When communication link is
 * created return a minisocket_t through which the communication can be made
 * from now on.
 *
 * The argument "port" is the port number on the local machine to which the
 * client will connect.
 *
 * Return value: the minisocket_t created, otherwise NULL with the errorcode
 * stored in the "error" variable.
 */
minisocket_t minisocket_server_create(int port, minisocket_error *error);

/*
 * Initiate the communication with a remote site. When communication is
 * established create a minisocket through which the communication can be made
 * from now on.
 *
 * The first argument is the network address of the remote machine. 
 *
 * The argument "port" is the port number on the remote machine to which the
 * connection is made. The port number of the local machine is one of the free
 * port numbers.
 *
 * Return value: the minisocket_t created, otherwise NULL with the errorcode
 * stored in the "error" variable.
 */
minisocket_t minisocket_client_create(network_address_t addr, int port, minisocket_error *error);

/* 
 * Send a message to the other end of the socket.
 *
 * The send call should block until the remote host has ACKnowledged receipt of
 * the message.  This does not necessarily imply that the application has called
 * 'minisocket_receive', only that the packet is buffered pending a future
 * receive.
 *
 * It is expected that the order of calls to 'minisocket_send' implies the order
 * in which the concatenated messages will be received.
 *
 * 'minisocket_send' should block until the whole message is reliably
 * transmitted or an error/timeout occurs
 *
 * Arguments: the socket on which the communication is made (socket), the
 *            message to be transmitted (msg) and its length (len).
 * Return value: returns the number of successfully transmitted bytes. Sets the
 *               error code and returns -1 if an error is encountered.
 */
int minisocket_send(minisocket_t socket, minimsg_t msg, int len, minisocket_error *error);

/*
 * Receive a message from the other end of the socket. Blocks until max_len
 * bytes or a full message is received (which can be smaller than max_len
 * bytes).
 *
 * Arguments: the socket on which the communication is made (socket), the memory
 *            location where the received message is returned (msg) and its
 *            maximum length (max_len).
 * Return value: -1 in case of error and sets the error code, the number of
 *           bytes received otherwise
 */
int minisocket_receive(minisocket_t socket, minimsg_t msg, int max_len, minisocket_error *error);

/* Close a connection. If minisocket_close is issued, any send or receive should
 * fail.  As soon as the other side knows about the close, it should fail any
 * send or receive in progress. The minisocket is destroyed by minisocket_close
 * function.  The function should never fail.
 */
void minisocket_close(minisocket_t socket); 

#endif /* __MINISOCKETS_H_ */
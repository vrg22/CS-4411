/*
 * network.c:
 *      This module paints the unix socket interface a pretty color.
 */

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <signal.h>
#include <unistd.h>
#include <ctype.h>

#include "defs.h"
#include "network.h"
#include "interrupts_private.h"
#include "minithread.h"
#include "random.h"

#define BCAST_ENABLED 0
#define BCAST_USE_TOPOLOGY_FILE 0
#define BCAST_ADDRESS "192.168.1.255"
#define BCAST_LOOPBACK 0
#define BCAST_TOPOLOGY_FILE "topology.txt"

#define BCAST_MAX_LINE_LEN 128
#define BCAST_MAX_ENTRIES 64
#define BCAST_MAX_NAME_LEN 64

#define MINIMSG_PORT 8086

#define NETWORK_INTERRUPT_TYPE 2

/*******************************************************************************
*  Private types and functions                                                 *
*******************************************************************************/

typedef struct {
  char name[BCAST_MAX_NAME_LEN];
  network_address_t addr;
  int links[BCAST_MAX_ENTRIES];
  int n_links;
} bcast_entry_t;

typedef struct {
  int n_entries;
  bcast_entry_t entries[BCAST_MAX_ENTRIES];
  int me;
} bcast_t;


bcast_t topology;

short my_udp_port = MINIMSG_PORT;
short other_udp_port = MINIMSG_PORT;

double loss_rate = 0.0;
double duplication_rate = 0.0;
int synthetic_network = 0;


struct address_info {
  int sock;
  struct sockaddr_in sin;
  char pkt[MAX_NETWORK_PKT_SIZE];
};

struct address_info if_info;
static network_address_t broadcast_addr = { 0 };

/* forward definition */
void start_network_poll(interrupt_handler_t, int*);
void network_address_to_sockaddr(network_address_t addr, struct sockaddr_in* sin);
void sockaddr_to_network_address(struct sockaddr_in* sin, network_address_t addr);

/* zero the address, so as to make it invalid */
void network_address_blankify(network_address_t addr) {
   addr[0]=addr[1]=0;
}

/** Copy address "original" to address "copy". */
void
network_address_copy(network_address_t original, network_address_t copy) {
  copy[0] = original[0];
  copy[1] = original[1];
}

/* Compare addresses. Return 1 if same, 0 if different. */
int
network_address_same(network_address_t a, network_address_t b) {
  return (a[0] == b[0] && a[1] == b[1]);
}

void
network_printaddr(network_address_t addr) {
  char name[40];
  network_format_address(addr, name, 40);
  printf("%s", name);
}

static int
send_pkt(network_address_t dest_address, 
         int hdr_len, char* hdr, 
         int data_len, char* data) {
  int cc;
  struct sockaddr_in sin;
  char* bufp;
  int sz, pktlen;
  
  pktlen = hdr_len + data_len;
  
  /* sanity checks */
  if (hdr_len < 0 || data_len < 0 || pktlen > MAX_NETWORK_PKT_SIZE)
    return 0;
  
  /*
   * Pull up the headers and data and stuff them into the output
   * packet with the
   * field sizes embedded.
   */
  
  bufp = if_info.pkt;
  
  sz = hdr_len;
  memcpy(bufp, hdr, sz);
  bufp += sz;
  
  sz = data_len;
  memcpy(bufp, data, sz);
  bufp += sz;
  
  network_address_to_sockaddr(dest_address, &sin);
  cc = sendto(if_info.sock,
              if_info.pkt,
              pktlen,
              0,
              (struct sockaddr *) &sin,
              sizeof(sin));
  
  return cc;
}

int 
network_send_pkt(network_address_t dest_address, int hdr_len, 
                 char* hdr, int data_len, char* data) {

  if (synthetic_network) {
    if(genrand() < loss_rate)
      return (hdr_len+data_len);

    if(genrand() < duplication_rate)
      send_pkt(dest_address, hdr_len, hdr, data_len, data);
  }

  return send_pkt(dest_address, hdr_len, hdr, data_len, data);
}

void
network_get_my_address(network_address_t my_address) {
  char hostname[64];
  assert(gethostname(hostname, 64) == 0);
  network_translate_hostname(hostname, my_address);
  my_address[1] = htons(my_udp_port);
}

int
network_translate_hostname(char* hostname, network_address_t address) {
  struct hostent* host;
  unsigned int iaddr;
  //printf("resolving name %s\n",hostname);
  if(isalpha(hostname[0])) {
          host = gethostbyname(hostname);
          if (host == NULL)
                return -1;
          else {
                address[0] = *((int *) host->h_addr);
                address[1] = htons(other_udp_port);
                //printf("address[0] = %x",address[0]);
                //printf("address[1] = %x",address[1]);
                return 0;
          }
  }
  else {
          iaddr = inet_addr(hostname);
          //printf("iaddr = %x\n",iaddr);
          address[0] = iaddr;
          address[1] = htons(other_udp_port);
          return 0;
   }  
}

int 
network_compare_network_addresses(network_address_t addr1,
                                  network_address_t addr2){
  return (addr1[0]==addr2[0] && addr1[1]==addr2[1]);
}

void
sockaddr_to_network_address(struct sockaddr_in* sin, network_address_t addr) {
  addr[0] = sin->sin_addr.s_addr;
  addr[1] = sin->sin_port;
}

void
network_address_to_sockaddr(network_address_t addr, struct sockaddr_in* sin) {
  memset(sin, 0, sizeof(*sin));
  sin->sin_addr.s_addr = addr[0];
  sin->sin_port = (short)addr[1];
  sin->sin_family = SOCK_DGRAM;
}

int 
network_format_address(network_address_t address, char* string, int length) {
  struct in_addr ipaddr;
  char* textaddr;
  int addrlen;
  
  ipaddr.s_addr = address[0];
  textaddr = inet_ntoa(ipaddr);
  addrlen = strlen(textaddr);

  if (length >= addrlen + 5) {
    strcpy(string, textaddr);
    string[addrlen] = ':';
    sprintf(string+addrlen+1, "%d", ntohs((short) address[1]));
    return 0;
  }
  else 
    return -1;
}

void
network_udp_ports(short myportnum, short otherportnum) {
  my_udp_port = myportnum;
  other_udp_port = otherportnum;
}

void
network_synthetic_params(double loss, double duplication) {
  synthetic_network = 1;        
  loss_rate = loss;
  duplication_rate = duplication;
}

void
bcast_initialize(char* configfile, bcast_t* bcast) {
  FILE* config = fopen(configfile, "r");
  char line[BCAST_MAX_LINE_LEN];
  int i = 0;
  char* rv;
  network_address_t my_addr;
  unsigned int my_ip_addr;

  network_get_my_address(my_addr);
  my_ip_addr = my_addr[0];

  while ((rv = fgets(line, BCAST_MAX_LINE_LEN, config)) != NULL) {
    if (line[0] == '\r' || line[0] == '\n')
      break;
        line[strlen(line)-1] = '\0';
    strcpy(bcast->entries[i].name, line);
    bcast->entries[i].n_links = 0;
    if (network_translate_hostname(line, bcast->entries[i].addr) != 0) {
      kprintf("Error: could not resolve hostname %s.\n", line);
      AbortOnCondition(1,"Crashing.");
    }
    if (bcast->entries[i].addr[0] == my_ip_addr)
      bcast->me = i;
    i++;
  }

  bcast->n_entries = i;
  

  if (rv != NULL)
    for (i=0; i<bcast->n_entries; i++) {
      int j;
      AbortOnCondition(fgets(line, BCAST_MAX_LINE_LEN, config) == NULL,
                       "Error: incomplete adjacency matrix.");

      for (j=0; j<bcast->n_entries; j++)
        if (i == j)
          ; /* avoid self-links */
        else if (line[j] != '.') {
          bcast->entries[i].links[bcast->entries[i].n_links] = j;
          bcast->entries[i].n_links++;
        }         
    }

  fclose(config);
}

int
hostname_to_entry(bcast_t* bcast, char* hostname) {
  network_address_t addr;
  unsigned int ipaddr;
  int entry = -1;
  int i;

  if (hostname == NULL)
    return bcast->me;

  if (network_translate_hostname(hostname, addr) != 0) {
    kprintf("Error: could not resolve host name.\n");
      AbortOnCondition(1,"Crashing.");
  }

  ipaddr = addr[0];

  for (i=0; i<bcast->n_entries; i++)
    if (ipaddr == bcast->entries[i].addr[0])
      entry = i;

  AbortOnCondition(entry == -1,
                   "Error: host name not in broadcast table.");

  return entry;
}

void
bcast_add_link(bcast_t* bcast, char* src, char* dest) {
  int srcnum, destnum;
  int i;

  srcnum = hostname_to_entry(bcast, src);
  destnum = hostname_to_entry(bcast, dest);

  for (i=0; i<bcast->entries[srcnum].n_links; i++)
    if (bcast->entries[srcnum].links[i] == destnum)
      return;

  bcast->entries[srcnum].links[bcast->entries[srcnum].n_links++] = destnum;
}

void
bcast_remove_link(bcast_t* bcast, char* src, char* dest) {
  int srcnum, destnum;
  int i;

  srcnum = hostname_to_entry(bcast, src);
  destnum = hostname_to_entry(bcast, dest);

  for (i=0; i<bcast->entries[srcnum].n_links; i++)
    if (bcast->entries[srcnum].links[i] == destnum) {
      if (i < bcast->entries[srcnum].n_links-1) {
        bcast->entries[srcnum].links[i] = 
          bcast->entries[srcnum].links[--bcast->entries[srcnum].n_links];
        break;
      }
      else
        bcast->entries[srcnum].n_links--;
    }  
}

int
network_bcast_pkt(int hdr_len, char* hdr, int data_len, char* data) {
  int i;
  int me;

  AbortOnCondition(!BCAST_ENABLED,
                   "Error: network broadcast not enabled.");
  
  if (BCAST_USE_TOPOLOGY_FILE){

    me = topology.me;
    
    for (i=0; i<topology.entries[me].n_links; i++) {
      int dest = topology.entries[me].links[i];
      
      if (synthetic_network) {
        if(genrand() < loss_rate)
          continue;
        
        if(genrand() < duplication_rate)
          send_pkt(topology.entries[dest].addr, hdr_len, hdr, data_len, data);
      }
      
      if (send_pkt(topology.entries[dest].addr, 
                   hdr_len, hdr, data_len, data) != hdr_len + data_len)
        return -1;
    }

    if (BCAST_LOOPBACK) {
      if (send_pkt(topology.entries[me].addr, 
                   hdr_len, hdr, data_len, data) != hdr_len + data_len)
        return -1;
    }
  
  } else { /* real broadcast */

    /* send the packet using the private network broadcast address */
    if (send_pkt(broadcast_addr, 
                 hdr_len, hdr, data_len, data) != hdr_len + data_len)
      return -1;

  }
  return hdr_len+data_len;
}

void
network_add_bcast_link(char* src, char* dest) {
  bcast_add_link(&topology, src, dest);
}

void
network_remove_bcast_link(char* src, char* dest) {
  bcast_remove_link(&topology, src, dest);
}


int network_poll(void* arg) {
  int* s;
  network_interrupt_arg_t* packet;
  struct sockaddr_in addr;
  unsigned int fromlen = sizeof(struct sockaddr_in);

  s = (int *) arg;

  for (;;) {

    /* we rely on run_user_handler to destroy this data structure */
    if (DEBUG)
      kprintf("NET:Allocating an incoming packet.\n");

    packet = 
      (network_interrupt_arg_t *) malloc(sizeof(network_interrupt_arg_t));
    assert(packet != NULL);
  
    packet->size = recvfrom(*s, packet->buffer, MAX_NETWORK_PKT_SIZE,
                            0, (struct sockaddr *) &addr, &fromlen);
    if (packet->size <= 0) {
      kprintf("NET:Error, %d.\n", errno);
      AbortOnCondition(1,"Crashing.");
    }
    else if (DEBUG)
      kprintf("NET:Received a packet, seqno %d.\n", ntohl(*((int *) packet->buffer)));
   
    assert(fromlen == sizeof(struct sockaddr_in));
    sockaddr_to_network_address(&addr, packet->sender);

    /* 
     * now we have filled in the arg to the network interrupt service routine,
     * so we have to get the user's thread to run it.
     */
    if (DEBUG)
      kprintf("NET:packet arrived.\n");
    send_interrupt(NETWORK_INTERRUPT_TYPE, mini_network_handler, (void*)packet);
  }     
}

/* 
 * start polling for network packets. this is separate so that clock interrupts
 * can be turned on without network interrupts. however, this function requires
 * that clock_init has been called!
 */
void start_network_poll(interrupt_handler_t network_handler, int* s) {
  pthread_t network_thread;
  sigset_t set;
  sigset_t old_set;
  struct sigaction sa;
  sigemptyset(&set);
  sigaddset(&set,SIGRTMAX-1);
  sigaddset(&set,SIGRTMAX-2);
  sigprocmask(SIG_BLOCK,&set,&old_set);

  /* create clock and return threads, but discard ids */
  AbortOnCondition(pthread_create(&network_thread, NULL, (void*)network_poll, s),
      "pthread");

  sa.sa_handler = (void*)handle_interrupt;
  sa.sa_flags = SA_SIGINFO | SA_RESTART | SA_ONSTACK; 
  sa.sa_sigaction= (void*)handle_interrupt;
  sigemptyset(&sa.sa_mask);
  sigaddset(&sa.sa_mask,SIGRTMAX-2);
  sigaddset(&sa.sa_mask,SIGRTMAX-1);
  if (sigaction(SIGRTMAX-2, &sa, NULL) == -1)
      AbortOnError(0);

  pthread_sigmask(SIG_SETMASK,&old_set,NULL);
}

int
network_initialize(network_handler_t network_handler) {
  int arg = 1;
  mini_network_handler=(interrupt_handler_t) network_handler;

  memset(&if_info, 0, sizeof(if_info));
  
  if_info.sock = socket(PF_INET, SOCK_DGRAM, 0);
  if (if_info.sock < 0)  {
    perror("socket");
    return -1;
  }

  if_info.sin.sin_family = SOCK_DGRAM;
  if_info.sin.sin_addr.s_addr = htonl(0);
  if_info.sin.sin_port = htons(my_udp_port);
  if (bind(if_info.sock, (struct sockaddr *) &if_info.sin, 
           sizeof(if_info.sin)) < 0)  {
    /* kprintf("Error: code %ld.\n", GetLastError());*/
    AbortOnError(0);
    perror("bind");
    return -1;
  }

  /* set for fast reuse */
  assert(setsockopt(if_info.sock, SOL_SOCKET, SO_REUSEADDR, 
                    (char *) &arg, sizeof(int)) == 0);

  if (BCAST_ENABLED)
    bcast_initialize(BCAST_TOPOLOGY_FILE, &topology);

  /*
   * Interrupts are handled through the caller's handler.
   */

  start_network_poll(mini_network_handler, &if_info.sock);

  return 0;
}


#ifndef __HASHTABLE_H__
#define __HASHTABLE_H__

#include "synch.h"
#include "network.h"
#include "alarm.h"

typedef struct cache_elem* cache_elem_t;
typedef struct cache_table* cache_table_t;

struct cache_elem {
	network_address_t dest;
	//char path[MAX_ROUTE_LENGTH][8];
	char path[20][8];
	int path_len;
	semaphore_t mutex; // Mutex on cache element
	semaphore_t timeout; // 12 sec discovery timeout alert
	alarm_t expire; // 3 sec expire alarm
	alarm_t reply; // 12 sec discovery timeout alarm
};

struct cache_table {
	int size;
	// int n; // Number of used elements
	//cache_elem_t table[SIZE_OF_ROUTE_CACHE];
	cache_elem_t table[20];
};


extern cache_table_t cache_table_new();

extern cache_elem_t cache_table_insert(cache_table_t table, network_address_t dest, char* path);

extern cache_elem_t cache_table_get(cache_table_t table, network_address_t dest);

extern int cache_table_remove(cache_table_t table, network_address_t dest);

#endif /*__HASHTABLE_H__*/
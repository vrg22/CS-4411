typedef struct cache_elem* cache_elem_t;
struct cache_elem {
	network_address_t dest;
	char path[MAX_ROUTE_LENGTH][8];
	semaphore_t mutex; // Mutex on cache element
	semaphore_t timeout; // 12 sec discovery timeout alert
	alarm_t expire; // 3 sec expire alarm
	alarm_t reply; // 12 sec discovery timeout alarm
};

typedef struct cache_table* cache_table_t;
struct cache_table {
	int size;
	// int n; // Number of used elements
	cache_elem_t table[SIZE_OF_ROUTE_CACHE];
};


extern cache_table_t cache_table_new();

extern int cache_table_insert(cache_table_t table, network_address_t dest, char* path);

extern cache_elem_t cache_table_get(network_address_t dest);
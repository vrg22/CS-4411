#include "hashtable.h"
#include "miniroute.h"

cache_table_t cache_table_new() {
	cache_table_t table;
	// int i;

	table = malloc(sizeof(struct cache_table));
	if (table == NULL) { // malloc() failed
        fprintf(stderr, "ERROR: cache_table_new() failed to malloc new cache_table_t\n");
        return NULL;
    }

    table->size = SIZE_OF_ROUTE_CACHE;
    
    return table;
}

cache_elem_t cache_table_insert(cache_table_t table, network_address_t dest, char* path) {
	cache_elem_t entry;
    int index, i;

	entry = malloc(sizeof(struct cache_elem));
	if (entry == NULL) { // malloc() failed
        fprintf(stderr, "ERROR: cache_table_insert() failed to malloc new cache_elem_t\n");
        return NULL;
    }

    network_address_copy(dest, entry->dest);
    entry->mutex = semaphore_create();
    semaphore_initialize(entry->mutex, 1);
    entry->timeout = semaphore_create();
    semaphore_initialize(entry->timeout, 0);
    entry->expire = NULL;
    entry->reply = NULL;

    index = hash_address(entry->dest) % table->size;

    i = 0;
    while (table->table[(index + i) % table->size] != NULL && i < table->size) {
        i++;
    }

    table->table[(index + i) % table->size] = entry;

    return entry;
}

cache_elem_t cache_table_get(cache_table_t table, network_address_t dest) {
    return NULL;    
}

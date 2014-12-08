
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "minithread.h"
#include "block_defs.h"

/* 
 * fsck.c
 *       This module checks the file system by ...
 */

void check_fs(char* diskname) {
	int read; //initialized;
	superblock_t superblk = NULL;

	// Read superblock from disk
	read = disk_read_block(&disk, 0, (char*) superblk);
	if (superblk == NULL){
		printf("NO REAL SUCCESS! - what to do here?\n");
		return;
	}



	if (read == 0) { // Success
		printf("SUCCESS!\n");
	} 
	else if (read == -1) { // Error
		printf("ERROR!\n");
		return;
	}
	else { // -2, too many requests??
		printf("-2: Too many requests?\n");
		return;
	}
}

int main(int argc, char** argv) {
	use_existing_disk = 1;
	disk_name = argv[1];

	printf("%s\n", argv[1]);

    minithread_system_initialize((proc_t) check_fs, (arg_t) argv[1]);
    return -1;
}
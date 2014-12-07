
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "minithread.h"
#include "block_defs.h"

/* 
 * mkfs.h
 *       This module sets up the file system by setting up the disk and its superblock.
 */

void make_fs(char* disksize) {
	int written;
	superblock_t superblk;
	// char* buffer = NULL;

	printf("size + 1 = %i\n", disk_size+1);

	//Create a new disk
	// disk = malloc(sizeof(disk_t));
	// if (disk == NULL) {
	// 	fprintf(stderr, "ERROR: mkfs.c failed to malloc disk\n");
	// }
	// disk_create(disk, disk_name, (int) *disksize, disk_flags); //disk is in minithread.c???

	
	//Create the superblock
	superblk = malloc(sizeof(struct superblock));
	if (superblk == NULL) {
		fprintf(stderr, "ERROR: mkfs.c failed to create disk's superblock\n");
	}
	memcpy(superblk->data.disk_size, disksize, 4);
	// strcpy(superblk->data.root_inode, disksize);


	//Write superblock to disk
	written = disk_write_block(&disk, 0, (char*) superblk);

	if (written == 0) { // Success
		printf("SUCCESS!\n");
	} 
	else if (written == -1) { // Error
		printf("ERROR!\n");
	}
	else { // -2, too many requests??
		printf("-2: Too many requests?\n");
	}

}

int main(int argc, char** argv) {
	use_existing_disk = 0;
	disk_name = "disk0";
	disk_flags = DISK_READWRITE;
	disk_size = (int) *argv[1];

    minithread_system_initialize((proc_t) make_fs, (arg_t) argv[1]); 		//fix cast!?
    return -1;
}
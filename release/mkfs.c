#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "minithread.h"
#include "block_defs.h"

/* 
 * mkfs.c
 * This module sets up the file system by setting up the disk and its superblock.
 */

void make_fs(char* disksize) {
	superblock_t superblk;

	use_existing_disk = 0;
	disk_name = "disk0";
	disk_flags = DISK_READWRITE;
	disk_size = (int) *disksize;

	// printf("size + 1 = %i\n", disk_size+1);

	//Create disk
	//Write superblock to disk

	//Create the superblock
	superblk = malloc(sizeof(struct superblock));
	if (superblk == NULL) {
		fprintf(stderr, "ERROR: mkfs.c failed to create disk's superblock\n");
	}
	memcpy(superblk->data.disk_size, disksize, 8);
	// strcpy(superblk->data.root_inode, disksize);
}

int main(int argc, char** argv) {
	minithread_system_initialize((proc_t) make_fs, (arg_t) argv[1]);
	return -1;
}
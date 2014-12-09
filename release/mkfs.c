
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "minithread.h"
#include "block_defs.h"
#include "miniheader.h"

/* 
 * mkfs.c
 *       This module sets up the file system by setting up the disk and its superblock.
 */

void make_fs(char* disksize) {
	int written, magic_no, size, first_data_block;
	int i;
	superblock_t superblk;
	inode_t ind;
	free_data_block_t fdb;
	// char* buffer = NULL;

	// Partition disk inode/data sections
	size = atoi(disksize);
	// size = unpack_unsigned_int(disksize);
	first_data_block = size / 10;

	fprintf(stderr, "%i\n", size);

	// Check minimum disk size
	if (size < 40) {
		fprintf(stderr, "ERROR: make_fs() requested to create a disk with too small size\n");
		return;
	}
	
	/* ALLOCATE SUPERBLOCK */
	// Create the superblock
	superblk = malloc(sizeof(struct superblock));
	if (superblk == NULL) {
		fprintf(stderr, "ERROR: mkfs.c failed to create disk's superblock\n");
	}

	magic_no = atoi("4411");
	printf("%i\n", magic_no);

	pack_unsigned_int(superblk->data.magic_number, 4411);
	pack_unsigned_int(superblk->data.disk_size, size);
	pack_unsigned_int(superblk->data.first_data_block, first_data_block);
	pack_unsigned_int(superblk->data.root_inode, 1);
	pack_unsigned_int(superblk->data.first_free_inode, 2);
	pack_unsigned_int(superblk->data.first_free_data_block, first_data_block);

	printf("Magic # = %i\n", unpack_unsigned_int(superblk->data.magic_number));
	printf("Root inode # = %i\n", unpack_unsigned_int(superblk->data.root_inode));
	// First free inode, first free data block

	// Write superblock to disk
	written = disk_write_block(&disk, 0, (char*) superblk);

	// Check for errors
	if (written == -1) { // Error
		fprintf(stderr, "ERROR!\n");
	} else if (written == -2) { // Too many requests?
		fprintf(stderr, "-2: Too many requests?\n");
	} else if (written != 0) {
		fprintf(stderr, "Something went wrong with disk_write_block() return value");
	}

	/* ALLOCATE INODE BLOCKS */
	for (i = 1; i < first_data_block; i++) {
		// Create new inode
		ind = malloc(sizeof(struct inode));
		if (ind == NULL) {
			fprintf(stderr, "ERROR: mkfs.c failed to create new inode\n");
		}

		// Write inode to disk
		written = disk_write_block(&disk, i, (char*) ind);

		// Check for errors
		if (written == -1) { // Error
			fprintf(stderr, "ERROR!\n");
		} else if (written == -2) { // Too many requests?
			fprintf(stderr, "-2: Too many requests?\n");
		} else if (written != 0) {
			fprintf(stderr, "Something went wrong with disk_write_block() return value");
		}
	}


	/* ALLOCATE DATA BLOCKS */
	for (i = first_data_block; i < size; i++) {
		// Create new db
		fdb = malloc(sizeof(struct free_data_block));
		if (fdb == NULL) {
			fprintf(stderr, "ERROR: mkfs.c failed to create new free_data_block\n");
		}

		// Write db to disk
		written = disk_write_block(&disk, i, (char*) ind);

		// Check for errors
		if (written == -1) { // Error
			fprintf(stderr, "ERROR!\n");
		} else if (written == -2) { // Too many requests?
			fprintf(stderr, "-2: Too many requests?\n");
		} else if (written != 0) {
			fprintf(stderr, "Something went wrong with disk_write_block() return value");
		}
	}

	if (written == 0) { // Success
		fprintf(stderr, "SUCCESS!\n");
	}
}

int main(int argc, char** argv) {
	use_existing_disk = 0;
	disk_name = "disk0";
	disk_flags = DISK_READWRITE;
	// disk_size = (int) *argv[1];
	disk_size = atoi(argv[1]);

	printf("%s\n", argv[1]);

	// minithread_system_initialize((proc_t) make_fs, (arg_t) disk_size); 		//fix cast!?
    minithread_system_initialize((proc_t) make_fs, (arg_t) argv[1]); 		//fix cast!?
    return -1;
}
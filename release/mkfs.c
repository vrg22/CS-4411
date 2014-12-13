
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
	dir_data_block_t ddb;
	// char* buffer = NULL;

	// Partition disk inode/data sections
	size = atoi(disksize);
	// size = unpack_unsigned_int(disksize);
	first_data_block = size / 10;

	fprintf(stderr, "%i\n", size);

	// Check minimum disk size
	if (size < 32) {
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
	// pack_unsigned_int(superblk->data.first_data_block, first_data_block);
	pack_unsigned_int(superblk->data.root_inode, 1);
	pack_unsigned_int(superblk->data.first_free_inode, 3);
	pack_unsigned_int(superblk->data.first_free_data_block, first_data_block + 1);
	pack_unsigned_int(superblk->data.free_inodes, first_data_block - 3);
	pack_unsigned_int(superblk->data.free_data_blocks, size - first_data_block - 1);

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

	/* ALLOCATE ROOT INODE BLOCK AND DIR_DATA_BLOCK */
	ind = malloc(sizeof(struct inode));
	if (ind == NULL) {
		fprintf(stderr, "ERROR: mkfs.c failed to create new inode\n");
	}
	ddb = malloc(sizeof(struct dir_data_block));
	if (ddb == NULL) {
		fprintf(stderr, "ERROR: mkfs.c failed to create new dir_data_block\n");
	}

	// Set inode data
	ind->data.inode_type = DIR;
	pack_unsigned_int(ind->data.size, 2);
	pack_unsigned_int(ind->data.next_free_inode, 0);
	pack_unsigned_int(ind->data.direct_ptrs[0], first_data_block);

	// Set ddb data
	pack_unsigned_int(ddb->data.inode_ptrs[0], 1); // . (current directory)
	memcpy(ddb->data.direct_entries[0], ".", 1);
	pack_unsigned_int(ddb->data.inode_ptrs[1], 1); // .. (parent directory)
	memcpy(ddb->data.direct_entries[1], "..", 2);

	// Write inode to disk
	written = disk_write_block(&disk, 1, (char*) ind);

	// Check for errors
	if (written == -1) { // Error
		fprintf(stderr, "ERROR!\n");
	} else if (written == -2) { // Too many requests?
		fprintf(stderr, "-2: Too many requests?\n");
	} else if (written != 0) {
		fprintf(stderr, "Something went wrong with disk_write_block() return value");
	} else {
		fprintf(stderr, "Wrote root inode successfully\n");
	}

	// Write ddb to disk
	written = disk_write_block(&disk, first_data_block, (char*) ddb);

	// Check for errors
	if (written == -1) { // Error
		fprintf(stderr, "ERROR!\n");
	} else if (written == -2) { // Too many requests?
		fprintf(stderr, "-2: Too many requests?\n");
	} else if (written != 0) {
		fprintf(stderr, "Something went wrong with disk_write_block() return value");
	} else {
		fprintf(stderr, "Wrote root ddb successfully\n");
	}

	/* ALLOCATE NON-ROOT INODE BLOCKS */
	for (i = 2; i < first_data_block; i++) {
		// Create new inode
		ind = malloc(sizeof(struct inode));
		if (ind == NULL) {
			fprintf(stderr, "ERROR: mkfs.c failed to create new inode\n");
		}

		// Set size = 0
		pack_unsigned_int(ind->data.size, 0);

		// Set next_free_inode
		if (i + 1 < first_data_block) {
			pack_unsigned_int(ind->data.next_free_inode, i + 1);
		} else {
			pack_unsigned_int(ind->data.next_free_inode, 0); // Last inode
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


	/* ALLOCATE NON-ROOT DATA BLOCKS */
	for (i = first_data_block + 1; i < size; i++) {
		// Create new db
		fdb = malloc(sizeof(struct free_data_block));
		if (fdb == NULL) {
			fprintf(stderr, "ERROR: mkfs.c failed to create new free_data_block\n");
		}

		// Set next_free_block
		if (i + 1 < size) {
			pack_unsigned_int(fdb->data.next_free_block, i + 1);
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

	while (1);
}

int main(int argc, char** argv) {
	use_existing_disk = 0;
	disk_name = "disk0";
	disk_flags = DISK_READWRITE;
	disk_size = atoi(argv[1]);

	printf("%s\n", argv[1]);

	// minithread_system_initialize((proc_t) make_fs, (arg_t) disk_size); 		//fix cast!?
    minithread_system_initialize((proc_t) make_fs, (arg_t) argv[1]); 		//fix cast!?
    return -1;
}
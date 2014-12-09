#ifndef __BLOCK_DEFS_H__
#define __BLOCK_DEFS_H__

#include "disk.h"


#define TABLE_SIZE 11 //FIX THIS LATER!!!!!!!!

typedef struct superblock {
	union {
		struct {
			char magic_number[4];
			char disk_size[4];
			char first_data_block[4];

			char root_inode[4];

			char first_free_inode[4];
			char first_free_data_block[4];
		} data;

		char padding[DISK_BLOCK_SIZE];
	};
} *superblock_t;

typedef struct inode {
	union {
		struct {
			char inode_type;
			char size[4];

			char direct_ptrs[TABLE_SIZE][4];
			char indirect_ptr[4];
		} data;

		char padding[DISK_BLOCK_SIZE];
	};
} *inode_t;

typedef struct dir_data_block {
	union {
		struct {
			char direct_entries[TABLE_SIZE][256]; // File name
			char inode_ptrs[TABLE_SIZE][4]; // Block # of corresponding file's "main" inode
		} data;

		char padding[DISK_BLOCK_SIZE];
	};
} *dir_data_block_t;

typedef struct free_data_block {
	union {
		char next_free_block[4];

		char padding[DISK_BLOCK_SIZE];
	};
} *free_data_block_t;

#endif /* __BLOCK_DEFS_H__ */
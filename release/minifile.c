#include <stdio.h>
#include "minifile.h"
#include "disk.h"
#include "synch.h"
#include "minithread.h"
#include "miniheader.h"

/*
 * struct minifile:
 *     This is the structure that keeps the information about 
 *     the opened file like the position of the cursor, etc.
 */

struct minifile {
	char name[256];
	file_mode_t mode;
	int inode_num; // Block # of inode
	int cursor;
	semaphore_t req_done; // Make sure disk request completes
	// int dirty; // ?????? 1 if contents differ from that is on disk, 0 if no changes have been made

	// char* read_buffer; //????
	// char* write_buffer; //?????
};

struct file_description {
	int inode_num; // Redundant (given from current index within file_description_table)
	int open_handles; // Number of open handles to this inode
	semaphore_t mutex; // Used only for reading & writing this file's inode or its data blocks
};

struct mutex_mem_buffer_map {
	char buff_addr[MAX_PENDING_DISK_REQUESTS][sizeof(char*)];
	char sema_addr[MAX_PENDING_DISK_REQUESTS][sizeof(semaphore_t*)];
};

semaphore_t metadata_mutex; // Mutex for metadata accesses spanning multiple inodes (creating, deleting files & directories that require directory inodes to change)
file_description_t* file_description_table;
semaphore_t fdt_mutex; // Mutex for file_description_table
mmbm_t requests; // Map for current requests
semaphore_t mmbm_sema; // Sema for getting entry in mmbm_t
semaphore_t mmbm_mutex; // Mutex for mmbm_t

// minifile_t* files; // All possible files indexed by corresponding inode

minifile_t minifile_creat(char *filename) {
	// Check for invalid arguments
	if (filename == NULL) {
		fprintf(stderr, "ERROR: minifile_creat() was passed a NULL filename pointer\n");
		return NULL;
	}

	return minifile_open(filename, "w");
}

minifile_t minifile_open(char *filename, char *mode) {
	minifile_t file;
	minithread_t thread;
	int dir, block_num;
	// char block_buffer[DISK_BLOCK_SIZE];
	// inode_t curr_inode;
	// dir_data_block_t dir_db;
	// indirect_block_t in;
	int plus = 0;

	// Check for invalid arguments
	if (filename == NULL) {
		fprintf(stderr, "ERROR: minifile_open() was passed a NULL filename pointer\n");
		return NULL;
	}
	if (mode == NULL) {
		fprintf(stderr, "ERROR: minifile_open() was passed a NULL mode pointer\n");
		return NULL;
	}

	// Check for unset file_description_table
	if (file_description_table == NULL) {
		file_description_table = malloc(disk_size / 10 * sizeof(file_description_t));
		if (file_description_table == NULL) {
			fprintf(stderr, "ERROR: minifile_open() failed to allocate file_description_table\n");
			return NULL;
		}
	}

	// Get disk and working directory
	thread = minithread_self();
	dir = thread->wd;

	// Attempt to open file
	if ((block_num = minifile_find(filename, dir)) < 0) { // No existing file exists

		// Create new inode

		// Add inode to current directory & update directory inode
	} else { // File already exists
		if (file found is a directory) {
			error & fail;
		}
	}

	// Update (or create) file description table entry
	semaphore_P(fdt_mutex);
	if (file_description_table[block_num] == NULL) {
		create file description table element;
		add element to table;
	}
	file_description_table[block_num]->open_handles++;
	semaphore_V(fdt_mutex);

	// Create minifile
	if ((file = malloc(sizeof(struct minifile))) == NULL) {
		fprintf(stderr, "ERROR: minifile_open() failed to create new minifile\n");
		return NULL;
	}
	file->name = filename; // COPY CORRECTLY!!!
	file->inode_num = block_num; // Block # of inode

	// Set file mode type - must consider 'b' intermixed with chars
	if (*mode == 'b') {
		mode++;
	}
	if (*(mode + 1) && *(mode + 1) == 'b') {
		if (*(mode + 2) && *(mode + 2) == '+') plus = 1;
	} else {
		if (*(mode + 1) && *(mode + 1) == '+') plus = 1;
	}
	if (*mode && *mode == 'r') {
		file->mode = plus ? RP : R;
	} else if (*mode && *mode == 'w') {
		file->mode = plus ? WP : W;
	} else if (*mode && *mode == 'a') {
		file->mode = plus ? AP : A;
	} else {
		fprintf(stderr, "ERROR: minifile_open() called with invalid mode type\n");
		return NULL;
	}
	// int cursor;



	// semaphore_P(file_description_table[block_num]->mutex);
	// read file into read buffer of minifile
	// semaphore_V(file_description_table[block_num]->mutex)

	return file;
}

int minifile_read(minifile_t file, char *data, int maxlen) {
	// Check for invalid arguments
	if (file == NULL) {
		fprintf(stderr, "ERROR: minifile_read() was passed a NULL file pointer\n");
		return -1;
	}

	return 0;
}

int minifile_write(minifile_t file, char *data, int len) {
	// Check for invalid arguments
	if (file == NULL) {
		fprintf(stderr, "ERROR: minifile_write() was passed a NULL file pointer\n");
		return -1;
	}

	return 0;
}

int minifile_close(minifile_t file) {
	// Check for invalid arguments
	if (file == NULL) {
		fprintf(stderr, "ERROR: minifile_close() was passed a NULL file pointer\n");
		return -1;
	}

	// flush file to disk

	free(file);

	return 0;
}

int minifile_unlink(char *filename) {
	// Check for invalid arguments
	if (filename == NULL) {
		fprintf(stderr, "ERROR: minifile_unlink() was passed a NULL filename pointer\n");
		return -1;
	}

	return 0;
}

int minifile_mkdir(char *dirname) {
	// Check for invalid arguments
	if (dirname == NULL) {
		fprintf(stderr, "ERROR: minifile_mkdir() was passed a NULL dirname pointer\n");
		return -1;
	}

	return 0;
}

int minifile_rmdir(char *dirname) {
	return 0;
}

int minifile_stat(char *path) {
	int block_num, error;
	char block_buffer[DISK_BLOCK_SIZE];
	inode_t ind;

	block_num = get_block_ptr(path); // Should point to the file's inode (guaranteed to be inode, hopefully)
	
	if (block_num < 0) { // Invalid path
		return -1;
	}

	// Lock fdt entry from other threads accessing this file while we read
	semaphore_P(fdt_mutex);
	semaphore_P(file_description_table[block_num]->mutex);
	semaphore_V(fdt_mutex);
	
	if ((error = minifile_read_block(&disk, block_num, block_buffer)) < 0) {
		fprintf(stderr, "ERROR: minifile_stat() failed to read block %i\n", error);
		return -1;
	}

	// Unlock fdt entry
	semaphore_P(fdt_mutex);
	semaphore_V(file_description_table[block_num]->mutex);
	semaphore_V(fdt_mutex);

	ind = (inode_t) block_buffer;

	if (ind->inode_type == DIR) {
		return -2;
	} else if (ind->inode_type == FIL) {
		return unpack_unsigned_int(ind->size);
	} else {
		fprintf(stderr, "ERROR: minifile_stat() got inode of unknown type\n");
		return -1;
	}
} 

int minifile_cd(char *path) {
	int block_num, error;
	char block_buffer[DISK_BLOCK_SIZE];
	inode_t ind;
	minithread_t thread;

	block_num = get_block_ptr(path); // Should point to the file's inode (guaranteed to be inode, hopefully)
	
	if (block_num < 0) { // Invalid path
		return -1;
	}

	// Lock fdt entry from other threads accessing this file while we read
	semaphore_P(fdt_mutex);
	semaphore_P(file_description_table[block_num]->mutex);
	semaphore_V(fdt_mutex);
	
	if ((error = minifile_read_block(&disk, block_num, block_buffer)) < 0) {
		fprintf(stderr, "ERROR: minifile_cd() failed to read block %i\n", error);
		return -1;
	}

	// Unlock fdt entry
	semaphore_P(fdt_mutex);
	semaphore_V(file_description_table[block_num]->mutex);
	semaphore_V(fdt_mutex);

	ind = (inode_t) block_buffer;

	if (ind->inode_type == DIR) {
		thread = minithread_self();
		thread->wd = block_num;
		return 0;
	} else if (ind->inode_type == FIL) {
		fprintf(stderr, "ERROR: minifile_cd() tried to access a non-directory\n");
		return -1;
	} else {
		fprintf(stderr, "ERROR: minifile_cd() got inode of unknown type\n");
		return -1;
	}
}

char **minifile_ls(char *path) {
	return NULL;
}

char* minifile_pwd(void) {
	return NULL;
}

int minifile_find(char *filename, int inode_block_num) {
	int error, block_num, file_not_found, entry, total_entries, db_num, i;
	char block_buffer[DISK_BLOCK_SIZE];
	inode_t curr_inode;
	dir_data_block_t dir_db;
	indirect_block_t in;

	// Check for invalid arguments
	if (filename == NULL) {
		fprintf(stderr, "ERROR: minifile_find() was passed a NULL filename pointer\n");
		return -1;
	}
	if (inode_block_num < 1) {
		fprintf(stderr, "ERROR: minifile_find() was passed an invalid inode_block_num\n");
		return -1;
	}

	// Get root inode
	if ((error = disk_read_block(&disk, inode_block_num, block_buffer)) < 0) {
		fprintf(stderr, "ERROR: minifile_find() failed to open current directory with error %i\n", error);
		return -1;
	}
	curr_inode = (inode_t) block_buffer;

	// Search for existing file with same name (does not distinguish between files & directories)
	file_not_found = 1;
	entry = 0; // Number of entries in directory searched
	total_entries = unpack_unsigned_int(curr_inode->data.size); // Number of entries in directory
	db_num = 0; // Current direct block pointer (from 0 to TABLE_SIZE - 1)
	while (db_num < TABLE_SIZE && entry < total_entries && file_not_found) { // Search through direct data blocks
		// Get next directory data block from root inode direct ptr
		if ((block_num = unpack_unsigned_int(curr_inode->data.direct_ptrs[db_num])) == 0) {
			fprintf(stderr, "ERROR: minifile_find() got block # of 0 for dir_db within indirect block with error %i\n", error);
			return -1;
		}
		if ((error = disk_read_block(&disk, block_num, block_buffer)) < 0) {
			fprintf(stderr, "ERROR: minifile_find() failed to open dir_db with error %i\n", error);
			return -1;
		}
		dir_db = (dir_data_block_t) block_buffer;

		i = 0;
		while (i < TABLE_SIZE && file_not_found) { // Scan direct data block entries for a match
			if (strcmp(dir_db->data.direct_entries[i], filename) == 0) { // FOUND MATCHING FILE
				file_not_found = 0;
			}

			entry++;
			i++;
		}

		db_num++;
	}

	block_num = unpack_unsigned_int(curr_inode->data.indirect_ptr);
	while (block_num != 0 && entry < total_entries && file_not_found) { // Search in indirect block
		// Get indirect block
		if ((error = disk_read_block(&disk, block_num, block_buffer)) < 0) {
			fprintf(stderr, "ERROR: minifile_find() failed to open indirect block with error %i\n", error);
			return -1;
		}
		in = (indirect_block_t) block_buffer;

		db_num = 0; // Current direct block pointer (from 0 to TABLE_SIZE - 1)
		while (db_num < TABLE_SIZE && entry < total_entries && file_not_found) { // Search through direct data blocks inside indirect block
			// Get next directory data block from root inode direct ptr
			if ((block_num = unpack_unsigned_int(curr_inode->data.direct_ptrs[db_num])) == 0) {
				fprintf(stderr, "ERROR: minifile_find() got block # of 0 for dir_db within indirect block with error %i\n", error);
				return -1;
			}
			if ((error = disk_read_block(&disk, block_num, block_buffer)) < 0) {
				fprintf(stderr, "ERROR: minifile_find() failed to open dir_db within indirect block with error %i\n", error);
				return -1;
			}
			dir_db = (dir_data_block_t) block_buffer;

			i = 0;
			while (i < TABLE_SIZE && file_not_found) { // Scan direct data block entries for a match
				if (strcmp(dir_db->data.direct_entries[i], filename) == 0) { // FOUND MATCHING FILE
					file_not_found = 0;
				}

				entry++;
				i++;
			}

			db_num++;
		}

		block_num = unpack_unsigned_int(in->data.indirect_ptr);
	}

	if (file_not_found) {
		return -1;
	} else {
		return block_num;
	}
}

int get_block_ptr(char *path) {
	minithread_t thread;
	int error, curr_block, i, file_name_chars;
	char block_buffer[DISK_BLOCK_SIZE];
	char file[256];

	// Check for invalid arguments
	if (path == NULL) {
		fprintf(stderr, "ERROR: get_block_ptr() was passed a NULL path\n");
		return -1;
	}

	thread = minithread_self();

	if (path[0] == '/') {
		if ((error = disk_read_block(&disk, 0, block_buffer)) < 0) { // Read superblock
			fprintf(stderr, "ERROR: get_block_ptr() failed to open superblock with error %i\n", error);
			return -1;
		}
		curr_block = unpack_unsigned_int(((superblock_t) block_buffer)->data.root_inode);
		i = 1;
	} else {
		curr_block = thread->wd;
		i = 0;
	}

	file_name_chars = 0;
	while (path[i]) {
		if (path[i] == '/') {
			curr_block = minifile_find(file, curr_block);
			file_name_chars = 0;
		} else {
			if (file_name_chars > 256) {
				fprintf(stderr, "ERROR: get_block_ptr() encountered file name with more than 256 chars\n");
				return -1;
			}
			file[file_name_chars] = path[i];
			file_name_chars++;
		}
		i++;
	}

	return curr_block;
}


int minifile_read_block(disk_t* disk, int blocknum, char* buffer) {
	semaphore_t sema;
	int status, entry;

	sema = semaphore_create(); 
	semaphore_initialize(sema, 0);

	// Block if not enough space in requests table
	semaphore_P(mmbm_sema);
	semaphore_P(mmbm_mutex);

	// Add mapping to table
	entry = 0;
	while (entry < MAX_PENDING_DISK_REQUESTS && requests->buff_addr[entry]) {
		entry++;
	}
	if (entry = MAX_PENDING_DISK_REQUESTS) {
		fprintf(stderr, "minifile_read_block ERROR\n");
		return -2;
	}

	requests->buff_addr[entry] = buffer; // COPY addr later
	requests->sema_addr[entry] = &sema; // COPY addr later
	semaphore_V(mmbm_mutex);

	status = disk_read_block(disk, blocknum, buffer);
	if (status < 0) {
		return status;
	}
	semaphore_P(sema);
	free(sema);

	// Know got disk reply
	semaphore_P(mmbm_mutex);
	requests->buff_addr[entry] = NULL; // COPY addr later
	requests->sema_addr[entry] = NULL;
	
	semaphore_V(mmbm_mutex);
	semaphore_V(mmbm_sema);

	return status;
}

/*
 * Encapsulates disk writes to also update map table, etc.
 */
int minifile_write_block(disk_t* disk, int blocknum, char* buffer){
	semaphore_t sema;
	int status, entry;

	sema = semaphore_create(); 
	semaphore_initialize(sema, 0);

	// Block if not enough space in requests table
	semaphore_P(mmbm_sema);
	semaphore_P(mmbm_mutex);

	// Add mapping to table
	entry = 0;
	while (entry < MAX_PENDING_DISK_REQUESTS && requests->buff_addr[entry]) {
		entry++;
	}
	if (entry = MAX_PENDING_DISK_REQUESTS) {
		fprintf(stderr, "minifile_write_block ERROR\n");
		return -2;
	}

	requests->buff_addr[entry] = buffer; // COPY addr later
	requests->sema_addr[entry] = &sema; // COPY addr later
	semaphore_V(mmbm_mutex);

	status = disk_write_block(disk, blocknum, buffer);
	if (status < 0) {
		return status;
	}
	semaphore_P(sema);
	free(sema);

	// Know got disk reply
	semaphore_P(mmbm_mutex);
	requests->buff_addr[entry] = NULL; // COPY addr later
	requests->sema_addr[entry] = NULL;
	
	semaphore_V(mmbm_mutex);
	semaphore_V(mmbm_sema);

	return status;
}
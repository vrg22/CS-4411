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
	file_mode_t mode;
	int open_handles;
};

minifile_t minifile_creat(char *filename) {
	minithread_t thread;
	minifile_t file;
	int dir, error, block_num, file_not_found, entry, total_entries, db_num, i;
	char block_buffer[DISK_BLOCK_SIZE];
	inode_t curr_inode;
	dir_data_block_t dir_db;
	indirect_block_t in;

	// Check for invalid arguments
	if (filename == NULL) {
		fprintf(stderr, "ERROR: minifile_creat() was passed a NULL filename pointer\n");
		return NULL;
	}

	// Get disk and working directory
	thread = minithread_self();
	dir = thread->wd;

	// Get root inode
	if ((error = disk_read_block(&disk, dir, block_buffer)) < 0) {
		fprintf(stderr, "ERROR: minifile_creat() failed to open current directory with error %i\n", error);
		return NULL;
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
			fprintf(stderr, "ERROR: minifile_creat() got block # of 0 for dir_db within indirect block with error %i\n", error);
			return NULL;
		}
		if ((error = disk_read_block(&disk, block_num, block_buffer)) < 0) {
			fprintf(stderr, "ERROR: minifile_creat() failed to open dir_db with error %i\n", error);
			return NULL;
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
			fprintf(stderr, "ERROR: minifile_creat() failed to open indirect block with error %i\n", error);
			return NULL;
		}
		in = (indirect_block_t) block_buffer;

		db_num = 0; // Current direct block pointer (from 0 to TABLE_SIZE - 1)
		while (db_num < TABLE_SIZE && entry < total_entries && file_not_found) { // Search through direct data blocks inside indirect block
			// Get next directory data block from root inode direct ptr
			if ((block_num = unpack_unsigned_int(curr_inode->data.direct_ptrs[db_num])) == 0) {
				fprintf(stderr, "ERROR: minifile_creat() got block # of 0 for dir_db within indirect block with error %i\n", error);
				return NULL;
			}
			if ((error = disk_read_block(&disk, block_num, block_buffer)) < 0) {
				fprintf(stderr, "ERROR: minifile_creat() failed to open dir_db within indirect block with error %i\n", error);
				return NULL;
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
		// Create new file
		if ((file = malloc(sizeof(struct minifile))) == NULL) {
			fprintf(stderr, "ERROR: minifile_creat() failed to create new minifile\n");
			return NULL;
		}

		// Create new inode

		// Add inode to current directory & update directory inode
	} else {
		// Use existing file
	}

	return file;
}

minifile_t minifile_open(char *filename, char *mode) {
	minifile_t file;
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

	// Open file
	file = minifile_find(filename);

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

	return NULL;
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
	return 0;
} 

int minifile_cd(char *path) {
	return 0;
}

char **minifile_ls(char *path) {
	return NULL;
}

char* minifile_pwd(void) {
	return NULL;
}


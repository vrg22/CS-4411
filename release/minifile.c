#include "minifile.h"
#include "disk.h"
#include "synch.h"

/*
 * struct minifile:
 *     This is the structure that keeps the information about 
 *     the opened file like the position of the cursor, etc.
 */

struct minifile {
	int open_handles;
};

minifile_t minifile_creat(char *filename) {
	// Check for invalid arguments
	if (filename == NULL) {
		fprintf(stderr, "ERROR: minifile_creat() was passed a NULL filename pointer\n");
		return NULL;
	}
}

minifile_t minifile_open(char *filename, char *mode) {
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
		return NULL;
	}

	return 0;
}

int minifile_write(minifile_t file, char *data, int len) {
	// Check for invalid arguments
	if (file == NULL) {
		fprintf(stderr, "ERROR: minifile_write() was passed a NULL file pointer\n");
		return NULL;
	}

	return 0;
}

int minifile_close(minifile_t file) {
	// Check for invalid arguments
	if (file == NULL) {
		fprintf(stderr, "ERROR: minifile_close() was passed a NULL file pointer\n");
		return NULL;
	}

	return 0;
}

int minifile_unlink(char *filename) {
	// Check for invalid arguments
	if (filename == NULL) {
		fprintf(stderr, "ERROR: minifile_unlink() was passed a NULL filename pointer\n");
		return NULL;
	}

	return 0;
}

int minifile_mkdir(char *dirname) {
	// Check for invalid arguments
	if (dirname == NULL) {
		fprintf(stderr, "ERROR: minifile_mkdir() was passed a NULL dirname pointer\n");
		return NULL;
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
	retun NULL;
}


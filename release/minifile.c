#include "minifile.h"
#include "disk.h"

/*
 * struct minifile:
 *     This is the structure that keeps the information about 
 *     the opened file like the position of the cursor, etc.
 */

struct minifile {
  /* add members here */
  int dummy;
};

minifile_t minifile_creat(char *filename) {
	return NULL;
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

	// Set file mode type
	if (*(mode + 1)) {
		plus = 1;
	}
	if (*mode == 'r') {
		file->mode = plus ? RP : R;
	} else if (*mode == 'w') {
		file->mode = plus ? WP : W;
	} else if (*mode == 'a') {
		file->mode = plus ? AP : A;
	} else {
		fprintf(stderr, "ERROR: minifile_open() called with invalid mode type\n");
		return NULL;
	}
}

int minifile_read(minifile_t file, char *data, int maxlen) {
	return 0;
}

int minifile_write(minifile_t file, char *data, int len) {
	return 0;
}

int minifile_close(minifile_t file) {
	return 0;
}

int minifile_unlink(char *filename) {
	return 0;
}

int minifile_mkdir(char *dirname) {
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


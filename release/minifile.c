#include "minifile.h"

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

}

int minifile_write(minifile_t file, char *data, int len) {

}

int minifile_close(minifile_t file) {

}

int minifile_unlink(char *filename) {

}

int minifile_mkdir(char *dirname) {

}

int minifile_rmdir(char *dirname) {

}

int minifile_stat(char *path) {

} 

int minifile_cd(char *path) {

}

char **minifile_ls(char *path) {

}

char* minifile_pwd(void) {

}


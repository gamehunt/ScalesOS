#include "dirent.h"
#include "errno.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#define R_RECURSIVE (1 << 0)
#define R_FORCE     (1 << 1)
#define R_PROMPT    (1 << 2)
#define R_EMPTY     (1 << 3)

void usage() {

}

int prompt(char* path) {
	printf("rm: remove %s? ", path);
	fflush(stdout);

	char r = fgetc(stdin);

	return tolower(r) == 'y';
}

int is_empty(char* dir) {
	DIR* d = opendir(dir);

	struct dirent* dent = NULL;

	int e = 1;

	while((dent = readdir(d))) {
		if(strcmp(dent->name, ".") && strcmp(dent->name, "..")) {
			e = 0;
			break;
		}
	}

	closedir(d);

	return e;
}

void do_remove(char* path, int dir, int p) {
	if(p && !prompt(path)) {
		return;
	}
	if(dir) {
		rmdir(path);
	} else {
		remove(path);
	}
}

int main(int argc, char** argv) {
	if(argc < 2) {
		usage();
	}

	int     flags = 0;
	char**  paths = 0;
	int     path_count = 0;

	for(int i = 1; i < argc; i++) {
		if(argv[i][0] != '-') {
			path_count++;
			if(!paths) {
				paths = malloc(sizeof(char*) * 1);
			} else {
				paths = realloc(paths, sizeof(char*) * path_count);
			}
			paths[path_count - 1] = argv[i];
		} else {
			for(size_t j = 1; j < strlen(argv[i]); j++) {
				switch(argv[i][j]) {
					case 'r':
						flags |= R_RECURSIVE;
						break;
					case 'i':
						flags |= R_PROMPT;
						break;
					case 'f':
						flags |= R_FORCE;
						break;
					case 'd':
						flags |= R_EMPTY;
						break;
				}
			}
		}
	}

	for(int i = 0; i < path_count; i++) {
		struct stat st;
		if(stat(paths[i], &st) < 0) {
			if(errno == ENOENT) {
				if(!(flags & R_FORCE)) {
					printf("rm: cannot remove '%s': no such file or directory\n", paths[i]);
				}
			} else {
					printf("rm: cannot stat '%s'\n", paths[i]);
			}
		} else {
			if(st.st_mode == S_IFDIR) {
				if(flags & R_EMPTY) {
					if(!is_empty(paths[i])) {
						printf("rm: cannot remove '%s': directory not empty.\n", paths[i]);
						continue;
					}
				} else if (!(flags & R_RECURSIVE)) {
					printf("rm: cannot remove '%s': is a directory\n", paths[i]);
						continue;
				}
			}
			do_remove(paths[i], st.st_mode == S_IFDIR, (flags & R_PROMPT) && !(flags & R_FORCE));
		}
	}

	return 0;
}

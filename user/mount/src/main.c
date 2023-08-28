#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mount.h>

static char* file = "/etc/filesystems";

static void usage() {

}

static void mount_all() {
	FILE*   fstab = fopen(file, "r");
	char    line[255];

	if(fstab) {
		while(fgets(line, 255, fstab)) {
			size_t size = strlen(line);

			if(line[size - 1] == '\n') {
				line[size - 1] = '\0';
			}

			char* word;
    		word = strtok(line, " ");
			char* args[3];
			int cont = 0;
			for(int i = 0; i < 3; i++) {
				if(!word) {
					fprintf(stderr, "Skipping malformed line: %s\r\n", line);
					cont = 1;
					break;
				}
				args[i] = malloc(strlen(word) + 1);
				strcpy(args[i], word);
				word = strtok(NULL, " ");
			}

			if(cont) {
				continue;
			}

			int reopen = 0;
			size_t offset = 0;
			if(!strcmp(args[0], "/")) {
				reopen = 1;
				offset = ftell(fstab);
				fclose(fstab);
				umount("/");
			}

			mount(args[0], args[1], args[2]);

			if(reopen) {
				fstab = fopen(file, "r");
				if(!fstab) {
					return;
				}
				fseek(fstab, offset, SEEK_SET);
			}

			for(int i = 0; i < 3; i++) {
				free(args[i]);
			}
		}
		fclose(fstab);
	} else {
		fprintf(stderr, "Invalid file.\r\n");
	}
}

int main(int argc, char** argv) {
	if(argc < 2) {
		usage();
		return 1;
	}

	if(!strcmp(argv[1], "-a")) {
		if(argc >= 3) {
			file = argv[3];
		}
		mount_all();
	}


	return 0;
}

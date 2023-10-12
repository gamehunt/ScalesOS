#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <string.h>

void dump(const char* path) {
	DIR* root = opendir(path);
	struct dirent* dirs = malloc(sizeof(struct dirent));
	int n = 0;
	int c = 1;
	if(root) {
		struct dirent* dir;
		while((dir = readdir(root))) {
			memcpy(&dirs[n], dir, sizeof(struct dirent));	
			n++;
			if(n >= c) {
				c *= 2;
				dirs = realloc(dirs, c * sizeof(struct dirent));	
			}
		}
		closedir(root);
	} else {
		printf("No such dir.\n");
	}

    for (int i = 0; i < n - 1; i++) {
        int swapped = 0;
        for (int j = 0; j < n - i - 1; j++) {
            if (strcmp(dirs[j].name, dirs[j + 1].name) > 0) {
				struct dirent tmp;
				memcpy(&tmp, &dirs[j], sizeof(struct dirent));
				memcpy(&dirs[j], &dirs[j + 1], sizeof(struct dirent));
				memcpy(&dirs[j + 1], &tmp, sizeof(struct dirent));
                swapped = 1;
            }
        }
        if (!swapped)
            break;
    }

	for(int i = 0; i < n; i++) {
		printf("%d %s\n", dirs[i].ino, dirs[i].name);
	}
}

int main(int argc, char** argv) {
	if(argc < 2) {
		dump(".");
	} else {
		dump(argv[1]);
	}
	return 0;
}

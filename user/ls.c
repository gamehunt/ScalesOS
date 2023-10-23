#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <string.h>

// This was made like this on purpose, for speed reasons, as I doubt,
// that we will have really big inodes
int inode_length(uint32_t ino) {
    if (ino < 0) ino = (ino == INT_MIN) ? INT_MAX : -ino;
    if (ino < 10) return 1;
    if (ino < 100) return 2;
    if (ino < 1000) return 3;
    if (ino < 10000) return 4;
    if (ino < 100000) return 5;
    if (ino < 1000000) return 6;
    if (ino < 10000000) return 7;
    if (ino < 100000000) return 8;
    if (ino < 1000000000) return 9;
    /*      2147483647 is 2^31-1 - add more ifs as needed
       and adjust this final return as well. */
    return 10;
}

void dump(const char* path) {
	DIR* root = opendir(path);
	struct dirent* dirs = malloc(sizeof(struct dirent));
	int n = 0;
	int c = 1;

	uint32_t longest_inode = 0;

	if(root) {
		struct dirent* dir;
		while((dir = readdir(root))) {
			if(dir->ino > longest_inode) {
				longest_inode = dir->ino;
			}
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

	int ino_width = inode_length(longest_inode);

	for(int i = 0; i < n; i++) {
		printf("%*d %s\n", ino_width, dirs[i].ino, dirs[i].name);
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

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/wait.h>
#include <dirent.h>
#include <sys/syscall.h>
#include <sys/mount.h>
#include <sys/reboot.h>
#include <sys/stat.h>
#include <scales/reboot.h>

void init_output() {
	__sys_open((uint32_t) "/dev/null", O_RDONLY, 0); // stdin
	__sys_open((uint32_t) "/dev/tty0", O_WRONLY, 0); // stdout
	__sys_open((uint32_t) "/dev/tty0", O_WRONLY, 0); // stderr
}

static uint8_t load_modules(const char* mod_path) {
	printf("Loading modules from %s...\r\n", mod_path);
	DIR*  dev = opendir(mod_path);
	char  path[1024];
	void* buffer;
	struct stat sb;
	if(dev) {
		struct dirent* dir;
		while((dir = readdir(dev))) {
			if(!strcmp(dir->name, ".") || !strcmp(dir->name, "..")) {
				continue;
			}
			sprintf(path, "%s/%s", mod_path, dir->name);
			FILE* f = fopen(path, "r");
			if(!f) {
				printf("Failed to open module: %s\r\n", path);
			} else{
				printf("Loading: %s...\r\n", path);

				if(fstat(fileno(f), &sb) < 0) {
					printf("--> Failed. \r\n");
					continue;
				}

				buffer = malloc(sb.st_size);
				fread(buffer, sb.st_size, 1, f);
				if(__sys_insmod((uint32_t) buffer, sb.st_size)){
					printf("--> Failed.\r\n");
				} else {
					printf("--> Ok.\r\n");
				}
				free(buffer);
			}
			fclose(f);
		}
		closedir(dev);
		return 0;
	} else {
		printf("Failed to open %s\r\n", mod_path);
		return 1;
	}
}


int execute(const char* path, char** argv, char** envp) {
	pid_t pid = fork();

	if(!pid) {
		execve(path, argv, envp);
		exit(1);
	}

	int status;
	waitpid(pid, &status, 0);
	
	return status;
}

void sort(char** arr, int n) {
    int i, j;
	char swapped;
    for (i = 0; i < n - 1; i++) {
        swapped = 0;
        for (j = 0; j < n - i - 1; j++) {
			int c = 0;
			size_t s1 = strlen(arr[j]);
			size_t s2 = strlen(arr[j + 1]);
			int max = s1 > s2 ? s2 : s1;
			while(c < max && arr[j][c] == arr[j + 1][c]) {
				c++;
			}
            if (arr[j][c] > arr[j + 1][c]) {
				char* tmp = arr[j];
				arr[j] = arr[j + 1];
				arr[j + 1] = tmp;
                swapped = 1;
            }
        }
  
        if (!swapped)
            break;
    }
}

int main(int argc, char** argv){
	init_output();

	if(load_modules("/modules")){
		printf("Errors occured during modules loading...\r\n");
	} else{
		printf("Loaded initrd modules.\r\n");
	}

	DIR* initd = opendir("/etc/init.d");
	if(initd) {
		seekdir(initd, 2);
		struct dirent* dir;
		char** scripts = malloc(sizeof(char*));
		int sc = 0;
		while((dir = readdir(initd))) {	
			char path[256];
			sprintf(path, "/etc/init.d/%s", dir->name);
			scripts[sc] = malloc(256);
			strcpy(scripts[sc], path);
			sc++;
			scripts = realloc(scripts, sizeof(char*) * (sc + 1));
		}
		closedir(initd);
		sort(scripts, sc);
		for(int i = 0; i < sc; i++) {
			printf("[%d/%d] Executing: %s\r\n", i, sc - 1, scripts[i]);
			int status = execute(scripts[i], 0, 0);
			if(status) {
				printf("--> Fail!\r\n");
			}
		}
	} else {
		printf("No init scripts found, falling back to getty.");
		execute("/bin/getty", NULL, NULL);
	}

	while(1);

	reboot(SCALES_REBOOT_CMD_REBOOT);
	__builtin_unreachable();
}

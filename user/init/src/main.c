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

void init_output() {
	__sys_open((uint32_t) "/dev/console", O_RDONLY, 0); // stdin
	__sys_open((uint32_t) "/dev/console", O_WRONLY, 0); // stdout
	__sys_open((uint32_t) "/dev/console", O_WRONLY, 0); // stderr
}

static uint8_t load_modules() {
	printf("Loading modules...\r\n");
	DIR*  dev = opendir("/modules");
	char  path[1024];
	void* buffer;
	if(dev) {
		seekdir(dev, 2);
		struct dirent* dir;
		while((dir = readdir(dev))) {
			sprintf(path, "/modules/%s", dir->name);
			FILE* f = fopen(path, "r");
			if(!f) {
				printf("Failed to open module: %s\r\n", path);
			} else{
				printf("Loading: %s...\r\n", path);
				uint32_t size = fseek(f, 0, SEEK_END);
				fseek(f, 0, SEEK_SET);	
				buffer = malloc(size);
				fread(buffer, size, 1, f);
				if(__sys_insmod((uint32_t) buffer, size)){
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
		printf("Failed to open /modules\r\n");
		return 1;
	}
}

void dump(const char* path) {
	printf("Gonna open %s\r\n", path);
	DIR* root = opendir(path);
	if(root) {
		printf("Dir opened, reading...\r\n");
		seekdir(root, 2);
		struct dirent* dir;
		while((dir = readdir(root))) {
			printf("%d %s\r\n", dir->ino, dir->name);
		}
		closedir(root);
	} else {
		printf("No such dir.\r\n");
	}
	printf("End\r\n");
}

pid_t execute(const char* path, char** argv, char** envp) {
	printf("Executing: %s\r\n", path);

	pid_t pid = fork();

	if(!pid) {
		execve(path, argv, envp);
		exit(1);
	}

	waitpid(pid, 0, 0);

	return pid;
}

extern int __get_argc();

int main(int argc, char** argv){
	init_output();

	printf("0x%.8x 0x%.8x\r\n", &argc, &argv);
	sleep(1);
	printf("0x%.8x 0x%.8x\r\n", &argc, &argv);
	usleep(500000);
	printf("0x%.8x 0x%.8x\r\n", &argc, &argv);

	while(1);

	if(load_modules()){
		printf("Errors occured during modules loading...\r\n");
	} else{
		printf("Loaded initrd modules.\r\n");
	}


	DIR* initd = opendir("/etc/init.d");
	if(initd) {
		struct dirent* dir;
		while((dir = readdir(initd))) {	
			if(!strcmp(dir->name, ".") || !strcmp(dir->name, "..")) {
				continue;
			}
			char path[256];
			sprintf(path, "/etc/init.d/%s", dir->name);
			execute(path, NULL, NULL);
		}
		closedir(initd);
	} else {
		printf("No init scripts found, falling back to getty.");
		execute("/bin/getty", NULL, NULL);
	}

	__sys_reboot();
	__builtin_unreachable();
}

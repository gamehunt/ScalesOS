#include "string.h"
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>

static FILE* accounts = NULL;
static struct passwd res;

struct passwd* getpwent(void) {
	if(!accounts) {
		accounts = fopen("/etc/accounts", "r");
	}

	if(!accounts || feof(accounts)) {
		return NULL;
	}

	char line[128];

	static char name[256];
	static char dir[256];
	static char shell[256];

	if(fgets(line, 128, accounts)) {
		strncpy(name, strtok(line, ":"), 256);
		res.pw_name  = name;
		res.pw_uid   = atoi(strtok(NULL, ":"));
		res.pw_gid   = atoi(strtok(NULL, ":"));
		strncpy(dir, strtok(NULL, ":"), 256);
		res.pw_dir   = dir;
		strncpy(shell, strtok(NULL, ":"), 256);
		res.pw_shell = shell;

		size_t len = strlen(res.pw_shell);
		if(res.pw_shell[len - 1] == '\n') {
			res.pw_shell[len - 1] = '\0';
		}
	} else {
		return NULL;
	}
	
	return &res;
}

void setpwent(void) {
	if(!accounts) {
		return;
	}
	fseek(accounts, 0, SEEK_SET);
}

void endpwent(void) {
	if(!accounts) {
		return;
	}
	fclose(accounts);
}

struct passwd* getpwnam(const char *name) {
	setpwent();
	while(getpwent()) {
		if(!strcmp(res.pw_name, name)) {
			return &res;
		}
	}
	return NULL;
}

struct passwd *getpwuid(uid_t uid) {
	setpwent();
	while(getpwent()) {
		if(res.pw_uid == uid) {
			return &res;
		}
	}
	return NULL;
}

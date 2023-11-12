#include "auth.h"

#include <stdio.h>
#include <pwd.h>
#include <string.h>
#include <stdlib.h>

static FILE* passwords = NULL;

int compare(char* passw, char* enc_passw) {
	return 0;
}

int auth_check_credentials(char* name, char* pass) {
	struct passwd* pwd = getpwnam(name);

	if(!pwd) {
		return -1;
	}

	if(!passwords) {
		passwords = fopen("/etc/passwords", "r");
		if(!passwords) {
			fprintf(stderr, "Failed to open /etc/passwords\n");
			return -2;
		}
	} else {
		fseek(passwords, SEEK_SET, 0);
	}

	char line[128];
	while(fgets(line, 128, passwords) != NULL) {
		char* word = strtok(line, ":");
		if(!strcmp(word, pwd->pw_name)) {
			char* enc_pass = strtok(NULL, ":");
			if(enc_pass && compare(pass, enc_pass)) {
				return -1;
			} else {
				break;
			}
		}
	}

	return pwd->pw_uid;
}

void auth_setenv(uid_t uid) {
	struct passwd* pwd = getpwuid(uid);

	if(!pwd) {
		return;
	}

	setenv("HOME",  pwd->pw_dir, 1);
	setenv("SHELL", pwd->pw_shell, 1);
}

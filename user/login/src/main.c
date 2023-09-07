#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int compare(char* passw, char* enc_passw) {
	return 0;
}

int try_login_with_password(int32_t uid, char* entry, char* name, char* pass) {
	static FILE* passwords = NULL;
	if(!passwords) {
		passwords = fopen("/etc/passwords", "r");
		if(!passwords) {
			fprintf(stderr, "Failed to open /etc/passwords\n");
			return -1;
		}
	} else {
		fseek(passwords, SEEK_SET, 0);
	}

	char line[128];
	while(fgets(line, 128, passwords) != NULL) {
		char* word = strtok(line, ":");
		if(!strcmp(word, name)) {
			char* enc_pass = strtok(NULL, ":");
			if(enc_pass && compare(pass, enc_pass)) {
				return 1;
			} else {
				break;
			}
		}
	}

	system("motd");

	int r = execve(entry, NULL, NULL);
	return -2;
}

int login(char* name, char* pass) {
	static FILE* accounts  = NULL;

	if(!accounts) {
		accounts = fopen("/etc/accounts", "r");
		if(!accounts) {
			fprintf(stderr, "Failed to open /etc/accounts\n");
			return -1;
		}
	} else {
		fseek(accounts, SEEK_SET, 0);
	}


	char line[128];
	while(fgets(line, 128, accounts) != NULL) {
		char* word = strtok(line, ":");
		if(!strcmp(word, name)) {
			char* uid   = strdup(strtok(NULL, ":"));
			char* guid  = strdup(strtok(NULL, ":"));
			char* home  = strdup(strtok(NULL, ":"));
			char* entry = strdup(strtok(NULL, ":"));

			if(entry[strlen(entry) - 1] == '\n') {
				entry[strlen(entry) - 1] = '\0';
			}

			int32_t iuid = atoi(uid);

			int r = try_login_with_password(iuid, entry, name, pass);

			free(uid);
			free(guid);
			free(home);
			free(entry);

			return r;
		}
	}

	return 1;
}

int main(int argc, char** argv) {
	printf("Login: ");
	fflush(stdout);
	
	char name[128];
	fgets(name, sizeof(name), stdin);

	char pass[128];
	while(1) {
		printf("Password: ");
		fflush(stdout);

		fgets(pass, sizeof(pass), stdin);

		name[strlen(name) - 1] = '\0';
		pass[strlen(pass) - 1] = '\0';

		int r = login(name, pass);
		
		if(r > 0) {
			printf("Incorrect login/password, please, try again\n");
		} else if (r < 0) {
			printf("Error occured during login, please, try again\n");
		}
	}

	return 0;
}

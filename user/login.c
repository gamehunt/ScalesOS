#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int compare(char* passw, char* enc_passw) {
	return 0;
}

int try_login_with_password(struct passwd* pwd, char* pass) {
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
		if(!strcmp(word, pwd->pw_name)) {
			char* enc_pass = strtok(NULL, ":");
			if(enc_pass && compare(pass, enc_pass)) {
				return 1;
			} else {
				break;
			}
		}
	}

	system("motd");

	chdir(pwd->pw_dir);
	int r = execve(pwd->pw_shell, NULL, NULL);

	return -2;
}

int login(char* name, char* pass) {
	
	struct passwd* pwd = getpwnam(name);

	if(!pwd) {
		return 1;
	}

	return try_login_with_password(pwd, pass);
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

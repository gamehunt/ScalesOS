#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/tty.h>

void echo() {
	struct termios t;
	tcgetattr(STDIN_FILENO, &t);
	t.c_lflag |= ECHO;
	tcsetattr(STDIN_FILENO, TCSANOW, &t);
}

void noecho() {
	struct termios t;
	tcgetattr(STDIN_FILENO, &t);
	t.c_lflag &= ~ECHO;
	tcsetattr(STDIN_FILENO, TCSANOW, &t);
}

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

	printf("\n");
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
	while(1) {
		char name[128];
		char pass[128];

		printf("Login: ");
		fflush(stdout);
		
		fgets(name, sizeof(name), stdin);

		printf("Password: ");
		fflush(stdout);

		noecho();

		fgets(pass, sizeof(pass), stdin);

		echo();

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

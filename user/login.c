#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/tty.h>

#include "auth.h"

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

		int r = auth_check_credentials(name, pass);
		
		if(r < 0) {
			if(r == -1) {
				printf("Incorrect login/password, please, try again\n");
			} else {
				printf("Error occured during login, please, try again\n");
			}
		}

		printf("\n");
		system("motd");

		auth_setenv(r);

		char* home  = getenv("HOME");
		char* shell = getenv("SHELL"); 

		chdir(home);
		execve(shell, NULL, NULL);
	}

	return 0;
}

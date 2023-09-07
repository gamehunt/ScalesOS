#include <stdio.h>

int main(int argc, char** argv) {
	printf("Login: ");
	fflush(stdout);
	char name[128];
	fgets(name, sizeof(name), stdin);
	printf("Your login: %s\n", name);
	return 0;
}

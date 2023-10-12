#include <stdio.h>
#include <unistd.h>

int main(int argc, char** argv) {
	printf("%s\n", get_current_dir_name());
	return 0;
}

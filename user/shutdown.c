#include <sys/reboot.h>
#include <scales/reboot.h>

int main(int argc, char** argv) {
	reboot(SCALES_REBOOT_CMD_SHUTDOWN);
	return 0;
}

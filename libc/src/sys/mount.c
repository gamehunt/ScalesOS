#include "sys/mount.h"
#include "sys/syscall.h"

int mount(const char* path, const char* dev, const char* type) {
	return __sys_mount((uint32_t) path, (uint32_t) dev, (uint32_t) type);
}

int umount(const char* path) {
	return __sys_umount((uint32_t) path);
}

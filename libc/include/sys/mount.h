#ifndef __SYS_MOUNT_H
#define __SYS_MOUNT_H

int mount(const char* path, const char* dev, const char* type);
int umount(const char* path);

#endif

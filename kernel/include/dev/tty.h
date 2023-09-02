#ifndef __K_DEV_TTY_H
#define __K_DEV_TTY_H

#include "kernel.h"
#include "kernel/fs/vfs.h"
#include "sys/tty.h"

K_STATUS k_dev_tty_init();
int      k_dev_tty_create_pty(struct winsize* ws, fs_node_t** master, fs_node_t** slave);

#endif

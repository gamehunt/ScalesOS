#include "dev/random.h"
#include "fs/vfs.h"
#include "kernel.h"
#include "util/log.h"

#define BYTE(v, n) ((v & (0xFF << (n * 8))) >> (n * 8))

uint32_t __k_dev_random_get_value_fallback() {
	static uint32_t next = 1;
	next = next * 1103515245 + 12345;
	return (uint32_t) (next / 65536) % 4294967296;
}

extern uint32_t __k_dev_random_get_value();

static uint32_t __k_dev_random_32bit() {
	uint32_t a = __k_dev_random_get_value();
	uint32_t b = __k_dev_random_get_value();

	return (a << 16) | (b & 0xFFFF);
}

static uint32_t __k_dev_random_read(fs_node_t* node UNUSED, uint32_t offset UNUSED, uint32_t size, uint8_t* buffer) {
	uint32_t full_bytes = size / 4;
	uint8_t  part_bytes = size % 4;

	for(uint32_t i = 0; i < full_bytes; i++) {
		*((uint32_t*) buffer) = __k_dev_random_32bit();
		buffer += 4;
	}

	if(part_bytes) {
		uint32_t value = __k_dev_random_get_value();

		for(uint32_t i = 0; i < part_bytes; i++) {
			buffer[i] = BYTE(value, i);
		}
	}

	return size;
}

static fs_node_t* __k_dev_random_create_device() {
	fs_node_t* node = k_fs_vfs_create_node("random");
	node->fs.read = __k_dev_random_read;

	return node;
}

K_STATUS k_dev_random_init() {
	k_fs_vfs_mount_node("/dev/random", __k_dev_random_create_device());
	return K_STATUS_OK;
}

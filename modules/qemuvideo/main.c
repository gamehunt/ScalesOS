#include "dev/pci.h"
#include "fs/vfs.h"
#include <kernel/mem/heap.h>
#include <kernel/mem/paging.h>
#include <kernel/mod/modules.h>
#include <kernel/kernel.h>
#include <kernel/util/log.h>

typedef struct qemu_lfb {
	uint8_t* buffer;
} qemu_lfb_t;

static pci_device_t* detect_qemu_lfb() {
	return k_dev_pci_find_device_by_vendor(0x1234, 0x1111);
}

static fs_node_t* __create_fb_device(pci_device_t* device) {
	fs_node_t*  dev = k_fs_vfs_create_node("fb");
	qemu_lfb_t* lfb = k_malloc(sizeof(qemu_lfb_t));
	lfb->buffer = (uint8_t*) (device->bars[0] & 0xFFFFFFFC);
	dev->device = lfb;
	return dev;
}

K_STATUS load(){
	pci_device_t* dev = detect_qemu_lfb();
	if(!dev) {
		k_err("QEMU lfb device not detected.");
		return K_STATUS_ERR_GENERIC;
	}

	k_fs_vfs_mount_node("/dev/fb", __create_fb_device(dev));
    return K_STATUS_OK;
}

K_STATUS unload(){
    return K_STATUS_OK;
}

MODULE("qemuvideo", &load, &unload)

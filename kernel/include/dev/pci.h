#ifndef __K_DEV_PCI_H
#define __K_DEV_PCI_H

#define PCI_ADDRESS 0xCF8
#define PCI_DATA    0xCFC

#include "kernel.h"

typedef struct {
	uint8_t bus;
	uint8_t slot;
	uint8_t func;
	uint16_t vendor;
	uint16_t device;
	uint8_t class;
	uint8_t subclass;
	uint32_t bars[6];
} pci_device_t;

K_STATUS k_dev_pci_init();
uint16_t k_dev_pci_read_word(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset);
void     k_dev_pci_write_word(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset, uint16_t value);

pci_device_t* k_dev_pci_find_device_by_class(uint8_t class, uint8_t subclass);
pci_device_t* k_dev_pci_find_device_by_pos(uint8_t bus, uint8_t slot, uint8_t func);
pci_device_t* k_dev_pci_find_device_by_vendor(uint16_t vendor, uint16_t device);

#endif

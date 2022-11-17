#ifndef __K_DEV_PCI_H
#define __K_DEV_PCI_H

#define PCI_ADDRESS 0xCF8
#define PCI_DATA    0xCFC

#include "kernel.h"

K_STATUS k_dev_pci_init();
uint16_t k_dev_pci_read_word(uint8_t bus, uint8_t slot, uint8_t func,
                             uint8_t offset);

#endif
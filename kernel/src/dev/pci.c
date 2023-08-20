#include <dev/pci.h>

#include "kernel.h"
#include "util/asm_wrappers.h"
#include "util/log.h"

K_STATUS k_dev_pci_init() {
    for (int bus = 0; bus < 256; bus++) {
        for (int slot = 0; slot < 256; slot++) {
            uint16_t vendor = k_dev_pci_read_word(bus, slot, 0, 0x0);
            if (vendor != 0xffff) {
                uint16_t device = k_dev_pci_read_word(bus, slot, 0, 0x2);
                uint16_t class = k_dev_pci_read_word(bus, slot, 0, 0xA);
                uint8_t header_type =
                    k_dev_pci_read_word(bus, slot, 0, 0xC) & 0xFF;
                uint16_t intr = k_dev_pci_read_word(bus, slot, 0, 0x3C);
                k_info(
                    "Found device: %d %d, Vendor: 0x%.4x Device: 0x%.4x MF: %d "
                    "HT: "
                    "0x%.2x",
                    bus, slot, vendor, device, header_type & 0x80,
                    header_type & 0x7F);
                k_info("--> Class: 0x%.2x Subclass: 0x%.2x",
                       (class & 0xFF00) >> 8, class & 0xFF);
                k_info("--> IRQ: %d Pin: 0x%.2x", intr & 0xFF,
                       (intr & 0xFF00) >> 8);
            }
        }
    }

    return K_STATUS_OK;
}

uint16_t k_dev_pci_read_word(uint8_t bus, uint8_t slot, uint8_t func,
                             uint8_t offset) {
    uint32_t address;
    uint32_t lbus = (uint32_t)bus;
    uint32_t lslot = (uint32_t)slot;
    uint32_t lfunc = (uint32_t)func;
    uint16_t tmp = 0;

    // Create configuration address as per Figure 1
    address = (uint32_t)((lbus << 16) | (lslot << 11) | (lfunc << 8) |
                         (offset & 0xFC) | ((uint32_t)0x80000000));

    // Write out the address
    outl(0xCF8, address);
    // Read in the data
    // (offset & 2) * 8) = 0 will choose the first word of the 32-bit register
    tmp = (uint16_t)((inl(0xCFC) >> ((offset & 2) * 8)) & 0xFFFF);
    return tmp;
}

void k_dev_pci_write_word(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset, uint16_t value) {
    uint32_t address;
    uint32_t lbus = (uint32_t)bus;
    uint32_t lslot = (uint32_t)slot;
    uint32_t lfunc = (uint32_t)func;

    // Create configuration address as per Figure 1
    address = (uint32_t)((lbus << 16) | (lslot << 11) | (lfunc << 8) |
                         (offset & 0xFC) | ((uint32_t)0x80000000));

    // Write out the address
    outl(0xCF8, address);

	// Write out value
	outl(0xCFC, value);
}

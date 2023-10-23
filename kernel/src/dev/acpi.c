#include "dev/pci.h"
#include "dev/rtc.h"
#include "kernel.h"
#include "mem/mmio.h"
#include "mem/paging.h"
#include "proc/smp.h"
#include "util/asm_wrappers.h"
#include "util/log.h"
#include "util/panic.h"
#include <dev/acpi.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <shared.h>

#include <mem/memory.h>

static uint32_t acpi_start = 0;

#define FROM_PHYS(addr) (acpi_start + ((addr) & 0xFFFF))

typedef struct rsdp_descriptor{
 char     signature[8];
 uint8_t  checksum;
 char     oemid[6];
 uint8_t  revision;
 uint32_t rsdt_address;
} __attribute__ ((packed)) rsdp_descriptor_t;

typedef struct acpi_sdt_header {
  char       signature[4];
  uint32_t   length;
  uint8_t    revision;
  uint8_t    checksum;
  char       oemid[6];
  char       oem_table_id[8];
  uint32_t   oem_revision;
  uint32_t   creator_id;
  uint32_t   creator_revision;
} acpi_sdt_header_t;

typedef struct rsdt {
  acpi_sdt_header_t header;
  uint32_t next_tables[];
} rsdt_t;

typedef struct generic_address_structure {
  uint8_t  address_space;
  uint8_t  bit_width;
  uint8_t  bit_offset;
  uint8_t  access_size;
  uint64_t address;
} generic_address_structure_t;

typedef struct fadt {
    acpi_sdt_header_t header;
    
    uint32_t firmware_ctrl;
    uint32_t dsdt;
 
    uint8_t  reserved;
 
    uint8_t  preferred_power_management_profile;
    uint16_t sci_interrupt;
    uint32_t smi_command_port;
    uint8_t  acpi_enable;
    uint8_t  acpi_disable;
    uint8_t  s4bios_req;
    uint8_t  pstate_control;
    uint32_t pm1a_event_block;
    uint32_t pm1b_event_block;
    uint32_t pm1a_control_block;
    uint32_t pm1b_control_block;
    uint32_t pm2_control_block;
    uint32_t pm_timer_block;
    uint32_t gpe0_block;
    uint32_t gpe1_block;
    uint8_t  pm1_gpeEventLength;
    uint8_t  pm1_control_length;
    uint8_t  pm2_control_length;
    uint8_t  pm_timer_length;
    uint8_t  gpe0_length;
    uint8_t  gpe1_length;
    uint8_t  gpe1_base;
    uint8_t  c_state_control;
    uint16_t worst_c2_latency;
    uint16_t worst_c3_latency;
    uint16_t flush_size;
    uint16_t flush_stride;
    uint8_t  duty_offset;
    uint8_t  duty_width;
    uint8_t  day_alarm;
    uint8_t  month_alarm;
    uint8_t  century;
 
    // reserved in ACPI 1.0; used since ACPI 2.0+
    uint16_t boot_architecture_flags;
 
    uint8_t  reserved2;
    uint32_t flags;
 
    // 12 byte structure; see below for details
    generic_address_structure_t reset_reg;
 
    uint8_t  reset_value;
    uint8_t  reserved3[3];
 
    // 64bit pointers - Available on ACPI 2.0+
    uint64_t                x_firmware_control;
    uint64_t                x_dsdt;
 
    generic_address_structure_t x_pm1a_event_block;
    generic_address_structure_t x_pm1b_event_block;
    generic_address_structure_t x_pm1a_control_block;
    generic_address_structure_t x_pm1b_control_block;
    generic_address_structure_t x_pm2_control_block;
    generic_address_structure_t x_pm_timer_block;
    generic_address_structure_t x_gpe0_block;
    generic_address_structure_t x_gpe1_block;
} fadt_t;

typedef struct local_apic_entry {
    uint8_t  acpi_id;
    uint8_t  apic_id;
    uint32_t flags;
}__attribute__ ((packed)) local_apic_entry_t;

typedef struct io_apic_entry {
    uint8_t  apic_id;
    uint8_t  reserved;
    uint32_t addr;
    uint32_t int_base;
}__attribute__ ((packed)) io_apic_entry_t;

typedef struct io_apic_int_src_entry {
    uint8_t  bus;
    uint8_t  irq;
    uint32_t glob;
    uint16_t flags;
}__attribute__ ((packed)) io_apic_int_src_entry_t;

typedef struct io_apic_nmi_entry {
    uint8_t  nmi;
    uint8_t  reserved;
    uint16_t flags;
    uint32_t glob;
}__attribute__ ((packed)) io_apic_nmi_entry_t;

typedef struct local_apic_nmi {
    uint8_t  acpi_id;
    uint16_t flags;
    uint8_t  lint;
}__attribute__ ((packed)) local_apic_nmi_t;

typedef struct madt_entry{
    uint8_t  type;
    uint8_t  length;
    uint8_t  data[];
}madt_entry_t;

typedef struct madt {
    acpi_sdt_header_t header;
    uint32_t local_apic_addr;
    uint32_t flags;
    madt_entry_t first_entry;
}madt_t;

static rsdp_descriptor_t*  descriptor = 0;
static rsdt_t* rsdt                   = 0;
static fadt_t* fadt                   = 0;

static uint32_t __k_dev_acpi_probe(uint32_t start, uint32_t end) {
    for (uint32_t i = start; i < end; i++) {
        char *str = (char *)i;
        if (!strncmp(str, "RSD PTR ", 8)) {
            return i;
        }
    }
    return 0;
}

static uint8_t __k_dev_acpi_verify_checksum(acpi_sdt_header_t* header){
    uint8_t* bytes = (uint8_t*) header;
    uint32_t summ  = 0;
    for(uint32_t i = 0; i < header->length; i++){
        summ += bytes[i];
    }

    return !(summ & 0xFF);
}

static uint8_t __k_dev_acpi_verify_rsdp_checksum(rsdp_descriptor_t header){
    uint8_t* bytes = (uint8_t*) &header;
    uint32_t summ  = 0;
    for(uint32_t i = 0; i < sizeof(header); i++){
        summ += bytes[i];
    }

    return !(summ & 0xFF);
}

static void __k_dev_acpi_traverse(rsdt_t* root){
    int entries = (root->header.length - sizeof(root->header)) / 4;
    for (int i = 0; i < entries; i++)
    {
        acpi_sdt_header_t *h = (acpi_sdt_header_t *) FROM_PHYS(root->next_tables[i]);
        k_info("%c%c%c%c: 0x%.8x %s", h->signature[0], h->signature[1], h->signature[2], h->signature[3], root->next_tables[i], __k_dev_acpi_verify_checksum(h) ? "" : "- Invalid!");
    }
}

static acpi_sdt_header_t* __k_dev_acpi_find(rsdt_t* root, const char* b){
    int entries = (root->header.length - sizeof(root->header)) / 4;
    for (int i = 0; i < entries; i++)
    {
        acpi_sdt_header_t *h = (acpi_sdt_header_t *) FROM_PHYS(root->next_tables[i]);
        if(__k_dev_acpi_verify_checksum(h) && !strncmp(h->signature, b, 4)){
            return h;
        }
    }
    return 0;
}

static void __k_dev_acpi_parse_aml(uint8_t* buffer, uint32_t length){

}

static void __k_dev_acpi_parse_madt(madt_t* madt){
    k_info("Local APIC: 0x%.8x Flags: 0x%.8x", madt->local_apic_addr, madt->flags);

    k_proc_smp_set_lapic_addr(madt->local_apic_addr);
    
    madt_entry_t* entry = &madt->first_entry;
    while((uint32_t)entry < (uint32_t)madt + madt->header.length){
        k_info("Entry type: %d (%d)", entry->type, entry->length);
        if(entry->type == 0){
            local_apic_entry_t* e = (local_apic_entry_t*) &entry->data[0];
            k_info("ACPI ID: 0x%.2x APIC ID: 0x%.2x FLAGS: 0x%.8x", e->acpi_id, e->apic_id, e->flags);
            k_proc_smp_add_cpu(e->acpi_id, e->apic_id);
        }else if(entry->type == 1){
            io_apic_entry_t*    e = (io_apic_entry_t*) &entry->data[0];
            k_info("-- IO APIC ID: 0x%.2x ADDR: 0x%.8x INT BASE: 0x%.8x", 
                e->apic_id, 
                e->addr,
                e->int_base
            );            
        }else if(entry->type == 2){
            io_apic_int_src_entry_t*    e = (io_apic_int_src_entry_t*) &entry->data[0];
            k_info("-- BUS: 0x%.2x IRQ: 0x%.2x GLOB INT: 0x%.8x FLAGS: 0x%.4x", 
                e->bus, 
                e->irq, 
                e->glob,
                e->flags
            );    
        }else if(entry->type == 3){
            io_apic_nmi_entry_t* e = (io_apic_nmi_entry_t*) &entry->data[0];
        }else if(entry->type == 4){
            local_apic_nmi_t* e = (local_apic_nmi_t*) &entry->data[0];
            k_info("-- ACPI ID: 0x%.2x FLAGS: 0x%.4x LINT: 0x%.2x", 
                   e->acpi_id, 
                   e->flags, 
                   e->lint
                ); 
        }
        entry = (madt_entry_t*)((uint32_t)entry + entry->length);
    }
}

K_STATUS k_dev_acpi_init() {
    if (!descriptor) {
        descriptor = (void*) __k_dev_acpi_probe(0xC0080000, 0xC0080400);
        if(!descriptor){
            descriptor = (void*) __k_dev_acpi_probe(0xC00E0000, 0xC00FFFFF);
        }
    }
    if (!descriptor) {
        k_err("ACPI tables not found");
        return K_STATUS_ERR_GENERIC;
    }

    k_info("ACPI tables found at 0x%.8x", descriptor);

    if(!__k_dev_acpi_verify_rsdp_checksum(*descriptor)){
        k_err("ACPI checksum verification failed");
        return K_STATUS_ERR_GENERIC;
    }

    k_info("ACPI Revision: %d", descriptor->revision);
    k_info("OEM ID: %s", descriptor->oemid);
    k_info("RSDT: 0x%.8x", descriptor->rsdt_address);

	acpi_start = k_map(descriptor->rsdt_address & 0xFFFF0000, 16, PAGE_PRESENT | PAGE_WRITABLE);

    rsdt = (rsdt_t*) FROM_PHYS(descriptor->rsdt_address);

    k_info("Available tables:");
    __k_dev_acpi_traverse(rsdt);

    fadt = (fadt_t*) __k_dev_acpi_find(rsdt, "FACP");

    if(!fadt){
        k_err("FADT not found");
        return K_STATUS_ERR_GENERIC; 
    }

    k_info("DSDT: 0x%.8x", fadt->dsdt);

    acpi_sdt_header_t* dsdt = (acpi_sdt_header_t*) FROM_PHYS(fadt->dsdt);
    if(!dsdt || !__k_dev_acpi_verify_checksum(dsdt)){
        k_err("DSDT not found");
        return K_STATUS_ERR_GENERIC; 
    }

    if(fadt->smi_command_port){
        outb(fadt->smi_command_port,fadt->acpi_enable);
        while ((inw(fadt->pm1a_control_block) & 1) == 0);
        k_info("ACPI enabled");
    }

    if(fadt->century){
        k_dev_rtc_enable_centrury();
    }
    
    __k_dev_acpi_parse_aml((uint8_t*) (dsdt+1), dsdt->length - sizeof(acpi_sdt_header_t));

    madt_t* madt = (madt_t*) __k_dev_acpi_find(rsdt, "APIC");

    if(madt){
        __k_dev_acpi_parse_madt(madt);
    }

    return K_STATUS_OK;
}

void k_dev_acpi_set_addr(void *buff) {
    descriptor = buff;
}

void k_dev_acpi_reboot() {
	if(descriptor->revision == 0) {
		k_debug("Reset register unsupported, using keyboard fallback.");
		uint8_t good = 0x02;
    	while (good & 0x02)
        	good = inb(0x64);
    	outb(0x64, 0xFE);
	} 
	switch(fadt->reset_reg.address_space) {
		case 0: /*Memory mapped*/
			*((uint8_t*)k_mem_mmio_map_register(fadt->reset_reg.address)) = fadt->reset_value;
		case 1: /*System IO*/
			outb(fadt->reset_reg.address, fadt->reset_value);
		case 2: /*PCI*/
			k_dev_pci_write_word(0, (fadt->reset_reg.address >> 32) & 0xFFFF, (fadt->reset_reg.address >> 16) & 0xFFFF, fadt->reset_reg.address & 0xFFFF, fadt->reset_value);
		default:
			k_panic("Reboot fail.", 0);
			__builtin_unreachable();
	}
}

void k_dev_acpi_shutdown() {
	outw(0x604, 0x2000); // TODO we need to parse AML and then do outw(PM1a_CNT, SLP_TYPa | SLP_EN );
						 // This will only work in QEMU
}

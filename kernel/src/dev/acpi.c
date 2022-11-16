#include "kernel.h"
#include "mem/paging.h"
#include "util/asm_wrappers.h"
#include "util/log.h"
#include <dev/acpi.h>
#include <stdio.h>
#include <string.h>

#define ACPI_MAPPING_START 0xF0000000

#define FROM_PHYS(addr) (ACPI_MAPPING_START + ((addr) & 0xFFFF))

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

    k_mem_paging_map_region(ACPI_MAPPING_START, descriptor->rsdt_address & 0xFFFF0000, 16, 0x3, 1);

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
    
    __k_dev_acpi_parse_aml((uint8_t*) (dsdt+1), dsdt->length - sizeof(acpi_sdt_header_t));

    return K_STATUS_OK;
}

void k_dev_acpi_set_addr(void *buff) {
    descriptor = buff;
}
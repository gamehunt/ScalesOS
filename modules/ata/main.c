#include <kernel/proc/mutex.h>
#include <kernel/mod/modules.h>
#include <kernel/proc/process.h>
#include <kernel/kernel.h>
#include <kernel/mem/heap.h>
#include <kernel/mem/mmio.h>
#include <kernel/mem/dma.h>
#include <kernel/mem/pmm.h>
#include <kernel/mem/paging.h>
#include <kernel/util/log.h>
#include <kernel/util/path.h>
#include <kernel/util/asm_wrappers.h>
#include <kernel/fs/vfs.h>
#include <kernel/dev/pci.h>
#include <kernel/int/pic.h>
#include <kernel/int/irq.h>

#include <types/list.h>

#include <stdio.h>
#include <string.h>

#define ATA_PRIMARY_IOBASE      0x1F0
#define ATA_SECONDARY_IOBASE    0x170

#define ATA_PRIMARY_CTRLBASE    0x3F6
#define ATA_SECONDARY_CTRLBASE  0x376

#define ATA_PRIMARY   0
#define ATA_SECONDARY 1

#define ATA_MASTER    0xA0
#define ATA_SLAVE     0xB0

#define ATA_IOBASE(type) ((type == ATA_PRIMARY) ? ATA_PRIMARY_IOBASE : ATA_SECONDARY_IOBASE)
#define ATA_CTRLBASE(type) ((type == ATA_PRIMARY) ? ATA_PRIMARY_CTRLBASE : ATA_SECONDARY_CTRLBASE) 

#define ATA_DATA(type)       (ATA_IOBASE(type) + 0)
#define ATA_ERROR(type)      (ATA_IOBASE(type) + 1)
#define ATA_FEAT(type)       (ATA_IOBASE(type) + 1)
#define ATA_SECCOUNT(type)   (ATA_IOBASE(type) + 2)
#define ATA_LBA_LO(type)     (ATA_IOBASE(type) + 3)
#define ATA_LBA_MID(type)    (ATA_IOBASE(type) + 4)
#define ATA_LBA_HI(type)     (ATA_IOBASE(type) + 5)
#define ATA_DRIVE(type)      (ATA_IOBASE(type) + 6)
#define ATA_STATUS(type)     (ATA_IOBASE(type) + 7)
#define ATA_COMMAND(type)    (ATA_IOBASE(type) + 7)

#define ATA_ALT_STATUS(type) (ATA_CTRLBASE(type) + 0)
#define ATA_DEV_CTRL(type)   (ATA_CTRLBASE(type) + 1)
#define ATA_DRIVE_ADDR(type) (ATA_CTRLBASE(type) + 1)

// 0	AMNF	Address mark not found.
// 1	TKZNF	Track zero not found.
// 2	ABRT	Aborted command.
// 3	MCR	Media change request.
// 4	IDNF	ID not found.
// 5	MC	Media changed.
// 6	UNC	Uncorrectable data error.
// 7	BBK	Bad Block detected.

#define ATA_ERR_AMNF  0
#define ATA_ERR_TKZNF 1
#define ATA_ERR_ABRT  2
#define ATA_ERR_MCR   3
#define ATA_ERR_IDNF  4
#define ATA_ERR_MC    5
#define ATA_ERR_UNC   6
#define ATA_ERR_BBK   7

// 0	ERR	Indicates an error occurred. Send a new command to clear it (or nuke it with a Software Reset).
// 1	IDX	Index. Always set to zero.
// 2	CORR	Corrected data. Always set to zero.
// 3	DRQ	Set when the drive has PIO data to transfer, or is ready to accept PIO data.
// 4	SRV	Overlapped Mode Service Request.
// 5	DF	Drive Fault Error (does not set ERR).
// 6	RDY	Bit is clear when drive is spun down, or after an error. Set otherwise.
// 7	BSY	Indicates the drive is preparing to send/receive data (wait for it to clear). In case of 'hang' (it never clears), do a software reset.

#define ATA_STATUS_ERR  (1 << 0)
#define ATA_STATUS_IDX  (1 << 1)
#define ATA_STATUS_CORR (1 << 2)
#define ATA_STATUS_DRQ  (1 << 3)
#define ATA_STATUS_SRV  (1 << 4)
#define ATA_STATUS_DF   (1 << 5)
#define ATA_STATUS_RDY  (1 << 6)
#define ATA_STATUS_BSY  (1 << 7)

typedef struct {
	uint32_t address;
	uint16_t size;
	uint16_t last;
} prdt_t;

typedef struct {
	uint8_t  bus;
	uint8_t  drive;
	uint8_t  lba48;
	uint8_t  udma_supported;
	uint8_t  udma_active;
	uint32_t lba28_sectors;
	uint64_t lba48_sectors;

	prdt_t*  prdt;
	uint32_t prdt_phys;

	list_t*  blocked_processes;

	mutex_t lock;
} drive_t;

char device_letter = 'a';

pci_device_t* __ide_controller = 0;

uint32_t busmaster_register = 0;
uint32_t busmaster_is_mmio  = 0;

list_t* ata_device_list = 0;

static drive_t* ata_get_device(uint8_t bus, uint8_t driv) {
	for(size_t i = 0; i < ata_device_list->size; i++) {
		drive_t* drive = ata_device_list->data[i];
		if(drive->bus == bus && drive->drive == driv) {
			return drive;
		}
	}
	return 0;
}

static void ata_wait(drive_t* device) {
	while (1) {
		uint8_t status = inb(ATA_ALT_STATUS(device->bus));
		if (!(status & ATA_STATUS_BSY) && (status & ATA_STATUS_RDY)) break;
	}
}

static uint8_t ata_read_busmaster_status(uint8_t bus) {
	if(busmaster_is_mmio) {
		return *((uint32_t*)(busmaster_register + 0x8 * bus + 2));
	} else {
		return inb(busmaster_register + 0x8 * bus + 2);
	}
}

static void ata_write_busmaster_status(uint8_t bus, uint8_t value) {
	if(busmaster_is_mmio) {
		*((uint32_t*)(busmaster_register + 0x8 * bus + 2)) = value;
	} else {
		outb(busmaster_register + 0x8 * bus + 2, value);
	}
}

static uint8_t ata_read_busmaster_command(uint8_t bus) {
	if(busmaster_is_mmio) {
		return *((uint8_t*)busmaster_register + 0x8 * bus);
	} else{
		return inb(busmaster_register + 0x8 * bus);
	}
}

static void ata_write_busmaster_command(uint8_t bus, uint8_t value) {
	if(busmaster_is_mmio) {
		*((uint8_t*)busmaster_register + 0x8 * bus) = value;
	} else{
		outb(busmaster_register + 0x8 * bus, value);
	}
}

static void ata_write_busmaster_prdt_address(uint8_t bus, uint32_t value) {
	if(busmaster_is_mmio) {
		*((uint32_t*)(busmaster_register + 0x8 * bus + 4)) = value;
	} else {
		outl(busmaster_register + 0x8 * bus + 4, value);
	}
}

static uint16_t* ata_identify(uint8_t bus, uint8_t drive) {
	outb(ATA_DRIVE(bus), drive);
	outb(ATA_LBA_LO(bus), 0);
	outb(ATA_LBA_MID(bus), 0);
	outb(ATA_LBA_HI(bus), 0);
	outb(ATA_COMMAND(bus), 0xEC);
	uint8_t status = inb(ATA_STATUS(bus));
	if(!status) {
		return 0;
	}

	while(status & (1 << 7)) {
		status = inb(ATA_STATUS(bus));
	}

	if(inb(ATA_LBA_MID(bus) || inb(ATA_LBA_HI(bus)))) {
		return 0;
	}

	status = 0;

	do {
		status = inb(ATA_STATUS(bus));
	} while(!(status & (1 << 3)) && !(status & (1 << 0)));

	if(status & (1 << 0)) {
		return 0;
	}

	static uint16_t identify[256];

	for(int i = 0; i < 256; i++) {
		identify[i] = inw(ATA_DATA(bus));
	}

	return &identify[0];
}

static const char* drivestr(uint8_t bus, uint8_t drive) {
	if(bus == ATA_PRIMARY) {
		if(drive == ATA_MASTER) {
			return "Primary bus, master drive";
		} else {
			return "Primary bus, slave drive";
		}
	} else{
		if(drive == ATA_MASTER) {
			return "Secondary bus, master drive";
		} else {
			return "Secondary bus, slave drive";
		}
	}
}

static void ata_initialize(drive_t* device) {
	device->prdt   = k_mem_dma_alloc(16, &device->prdt_phys);

	device->blocked_processes = list_create();

	list_push_back(ata_device_list, device);
}

#define BYTES_PER_PRD     (KB(64))
#define PRDS_PER_CLUSTER  (256 * 512 / BYTES_PER_PRD) 

// TODO FIXME
// This won't work for any dma_size exceeding one prd
static void __ata_prepare_prdt(drive_t* device, uint8_t* buffer, uint32_t dma_size) {
	uint32_t prdt_entries = dma_size * 512 / BYTES_PER_PRD + 1;
	for(uint32_t i = 0; i < prdt_entries; i++) {
		uint32_t left = dma_size * 512 - i * BYTES_PER_PRD;
		if(!left) {
			break;
		}
		device->prdt[i].address = k_mem_paging_virt2phys((uint32_t) buffer + i * BYTES_PER_PRD);
		if(left > BYTES_PER_PRD) {
			device->prdt[i].size = 0;
			device->prdt[i].last = 0x0;
		} else{
			if(left == BYTES_PER_PRD) {
				device->prdt[i].size = 0;
			} else {
				device->prdt[i].size = left;
			}
			device->prdt[i].last = 0x8000;
		}
		// k_debug("prdt[%d] = 0x%.8x +%d 0x%x", i, device->prdt[i].address, device->prdt[i].size, device->prdt[i].last);
	}
}

// TODO FIXME
// This won't work for any dma_size exceeding one prd
static uint32_t __ata_read_internal(drive_t* device, uint32_t lba, uint32_t dma_size, uint8_t* buffer) {
	mutex_lock(&device->lock);

	__ata_prepare_prdt(device, buffer, dma_size);

	ata_write_busmaster_command(device->bus, 0x0);

	ata_write_busmaster_prdt_address(device->bus, device->prdt_phys);
	
	ata_write_busmaster_status(device->bus, 0x4 | 0x2);
	ata_write_busmaster_command(device->bus, 0x8);

	outb(ATA_DEV_CTRL(device->bus), 0);
	outb(ATA_DRIVE(device->bus), device->drive == ATA_MASTER ? 0xe0 : 0xf0);

	ata_wait(device);

	outb(ATA_FEAT(device->bus), 0);
	ata_write_busmaster_command(device->bus, 0x8 | 0x1);

	int i = 0;
	while(dma_size) {
		size_t read = (device->prdt[i].size ? device->prdt[i].size : BYTES_PER_PRD) / 512;

		// k_debug("read: %d, dma left: %d, lba: +%d", read, dma_size, lba);
		// k_debug("reading 0x%.8x - 0x%.8x", device->prdt[i].address, device->prdt[i].address + read * 512);

		outb(ATA_SECCOUNT(device->bus), read);
		outb(ATA_LBA_LO(device->bus),  (uint8_t) (lba & 0xFF));
		outb(ATA_LBA_MID(device->bus), (uint8_t) ((lba & 0xFF00) >> 8));
		outb(ATA_LBA_HI(device->bus),  (uint8_t) ((lba & 0xFF0000) >> 16));
		ata_wait(device);

		outb(ATA_COMMAND(device->bus), 0xC8);

		k_proc_process_sleep_on_queue(k_proc_process_current(), device->blocked_processes);

		dma_size -= read;
		lba += read;
		i++;
	}

	ata_write_busmaster_command(device->bus, 0x0);

	mutex_unlock(&device->lock);
	
	return 0;
}

static uint32_t ata_read(fs_node_t* node, uint32_t offset, uint32_t size, uint8_t* buffer) {
	if(!size) {
		return 0;
	}

	if(offset > node->size) {
		return 0;
	}

	if(size + offset >= node->size) {
		size = node->size - offset;
	}

	drive_t* device = (drive_t*) node->inode;

	uint32_t lba_offset  = offset / 512;
	uint32_t part_offset = offset % 512;

	uint32_t full_sectors = size / 512;
	uint32_t last_sector  = size % 512 - part_offset;

	uint32_t dma_size = full_sectors + (last_sector > 0) + (part_offset > 0);

	uint32_t internal_buffer_size = 0;
	void*    internal_buffer = NULL;

	if(size <= BYTES_PER_PRD) {
		internal_buffer_size = size;
	} else {
		internal_buffer_size = BYTES_PER_PRD;
	}

	internal_buffer =  k_mem_dma_alloc(internal_buffer_size / 0x1000 + 1, NULL);

	uint32_t buffer_offset = 0;

	while(dma_size) {
		uint32_t read_size = internal_buffer_size / 512;
		if(read_size > dma_size) {
			read_size = dma_size;	
		}
		__ata_read_internal(device, lba_offset + buffer_offset / 512, read_size, internal_buffer);
		memcpy(&buffer[buffer_offset], &internal_buffer[part_offset], read_size * 512);
		part_offset   = 0;
		buffer_offset += read_size * 512;
		dma_size      -= read_size;
	}

	k_mem_dma_free(internal_buffer, internal_buffer_size / 0x1000 + 1);

	return size;
}

static uint32_t __ata_write_internal(drive_t* device, uint32_t lba, uint32_t dma_size, uint8_t* buffer) {
	mutex_lock(&device->lock);

	__ata_prepare_prdt(device, buffer, dma_size);

	ata_write_busmaster_command(device->bus, 0x0);

	ata_write_busmaster_prdt_address(device->bus, device->prdt_phys);
	
	ata_write_busmaster_status(device->bus, 0x4 | 0x2);
	ata_write_busmaster_command(device->bus, 0x8);

	outb(ATA_DEV_CTRL(device->bus), 0);
	outb(ATA_DRIVE(device->bus), device->drive == ATA_MASTER ? 0xe0 : 0xf0);

	ata_wait(device);

	outb(ATA_FEAT(device->bus), 0);

	outb(ATA_SECCOUNT(device->bus), dma_size);
	outb(ATA_LBA_LO(device->bus),  (uint8_t) (lba & 0xFF));
	outb(ATA_LBA_MID(device->bus), (uint8_t) ((lba & 0xFF00) >> 8));
	outb(ATA_LBA_HI(device->bus),  (uint8_t) ((lba & 0xFF0000) >> 16));
	
	ata_wait(device);

	outb(ATA_COMMAND(device->bus), 0xCA);

	ata_write_busmaster_command(device->bus, 0x8 | 0x1);

	k_proc_process_sleep_on_queue(k_proc_process_current(), device->blocked_processes);

	mutex_unlock(&device->lock);
	
	return 0;
}

static uint32_t ata_write(fs_node_t* node, uint32_t offset, uint32_t size, uint8_t* buffer) {
	if(!size) {
		return 0;
	}

	if(offset > node->size) {
		return 0;
	}

	drive_t* device = (drive_t*) node->inode;

	uint32_t lba_offset  = offset / 512;
	uint32_t part_offset = offset % 512;

	uint32_t full_sectors = size / 512;
	uint32_t last_sector  = size % 512 - part_offset;

	uint32_t dma_size = full_sectors + (last_sector > 0) + (part_offset > 0);

	void* internal_buffer = k_mem_dma_alloc(dma_size * 512 / 0x1000 + 1, NULL);

	memcpy(internal_buffer + part_offset, buffer, size);

	if(part_offset) {
		__ata_read_internal(device, lba_offset, 512, internal_buffer);
	}
	memcpy(buffer, &internal_buffer[part_offset], size);
	if(last_sector) {
		__ata_read_internal(device, lba_offset + dma_size - 1, 512, internal_buffer + (dma_size - 1) * 512);
	}

	__ata_write_internal(device, lba_offset, dma_size, internal_buffer);

	k_mem_dma_free(internal_buffer, dma_size * 512 / 0x1000 + 1);

	return size;
}
static void detect_drives() {
	for(int i = 0; i <= 1; i++) {
		for(int j = 0; j <= 1; j++) {
			uint8_t bus    = !i ? ATA_PRIMARY : ATA_SECONDARY;
			uint8_t drive  = !j ? ATA_MASTER : ATA_SLAVE;
			uint16_t* drive_info = ata_identify(bus, drive);
			if(drive_info) {
				drive_t* device = k_calloc(1, sizeof(drive_t));	
				device->drive = drive;
				device->bus   = bus;
				device->lba48 = drive_info[83] & (1 << 10);
				device->udma_supported  = drive_info[88] & 0xFF;
				device->udma_active     = (drive_info[88] & 0xFF00) >> 8;
				device->lba28_sectors = *((uint32_t*)&drive_info[60]);
				device->lba48_sectors = *((uint64_t*)&drive_info[100]);

				mutex_init(&device->lock);

				k_info("Drive found: %s", drivestr(bus, drive));
				k_info("LBA48: %d", device->lba48);
				k_info("UDMA status: %b %b", device->udma_active, device->udma_supported);
				k_info("LBA28 sectors: %ld", device->lba28_sectors);
				k_info("LBA48 sectors: %lld", device->lba48_sectors);

				ata_initialize(device);

				char device_path[64];
				sprintf(device_path, "/dev/hd%c", device_letter);
				device_letter++;

				char* device_name = k_util_path_filename(device_path);

				fs_node_t* node = k_fs_vfs_create_node(device_name);
				k_free(device_name);

				node->inode = (uint32_t) device;
				node->size = device->lba28_sectors * 512;
				node->fs.read  = &ata_read;
				node->fs.write = &ata_write;

				k_fs_vfs_mount_node(device_path, node);

				k_info("Mounted as %s", device_path);
			}
		}
	}
}

static interrupt_context_t* ata_handle_irq(uint8_t bus, interrupt_context_t* ctx) {
	uint8_t status = ata_read_busmaster_status(bus);

	if (!(status & 4)) {
		return ctx;
	}

	if((status & 2)) {
		k_err("ATA: I/O error.");
		return ctx;
	}

	// k_debug("ATA IRQ, done=%d (%d status)", !(status & 1), status);
	ata_write_busmaster_status(bus, 0x4 | 0x2);

	do {
		status = inb(ATA_STATUS(bus));
	} while(status & ATA_STATUS_BSY);

	uint8_t drive_selected = inb(ATA_DRIVE(bus)) & (1 << 4);
	drive_t* drive = ata_get_device(bus, drive_selected ? ATA_SLAVE : ATA_MASTER);
	if(drive) {
		k_proc_process_wakeup_queue(drive->blocked_processes);
	}

	return ctx;
}

static interrupt_context_t* ata_irq14_handler(interrupt_context_t* ctx) {
	ctx = ata_handle_irq(ATA_PRIMARY, ctx);
	k_int_pic_eoi(14);
	return ctx;
}

static interrupt_context_t* ata_irq15_handler(interrupt_context_t* ctx) {
	ctx = ata_handle_irq(ATA_SECONDARY, ctx);
	k_int_pic_eoi(15);
	return ctx;
}

K_STATUS load(){
	ata_device_list = list_create();

	__ide_controller = k_dev_pci_find_device_by_class(0x1, 0x1);
	if(!__ide_controller) {
		k_err("Failed to find IDE controller");
		return K_STATUS_ERR_GENERIC;
	} else {
		k_info("IDE controller found: 0x%.4x:0x%.4x", __ide_controller->vendor, __ide_controller->device);

		uint16_t command_reg = k_dev_pci_read_word(__ide_controller->bus, __ide_controller->slot, __ide_controller->func, 0x4);
		if (!(command_reg & (1 << 2))) {
			command_reg |= (1 << 2); 
			k_info("Enabled bus mastering.");
			k_dev_pci_write_word(__ide_controller->bus, __ide_controller->slot, __ide_controller->func, 0x4, command_reg);
		}

		uint32_t bar4 = __ide_controller->bars[4];
		if(bar4 & 1) {
			busmaster_is_mmio  = 0;
			busmaster_register = bar4 & 0xFFFFFFF0;
		} else {
			busmaster_is_mmio  = 1;
			busmaster_register = k_mem_mmio_map_register(bar4 & 0xFFFFFFF0);
		}
	}

	k_info("Busmaster: 0x%x, mode: mmio=%s", busmaster_register, (busmaster_is_mmio ? "true" : "false"));

	k_int_irq_setup_handler(14, &ata_irq14_handler);
	k_int_irq_setup_handler(15, &ata_irq15_handler);
	
	k_int_pic_unmask_irq(14);
	k_int_pic_unmask_irq(15);

	detect_drives();

    return K_STATUS_OK;
}

K_STATUS unload(){
    return K_STATUS_OK;
}

MODULE("ata", &load, &unload)

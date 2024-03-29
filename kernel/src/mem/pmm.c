#include "dev/acpi.h"
#include "mem/memory.h"
#include "mem/paging.h"
#include "multiboot.h"
#include "proc/spinlock.h"
#include "shared.h"
#include "util/asm_wrappers.h"
#include "util/log.h"
#include "util/panic.h"

#include <mem/pmm.h>
#include <stdio.h>

#define BITMAP_INDEX(frame) (frame / 0x1000 / 32)
#define BITMAP_BIT_NUMBER(frame) ((frame / 0x1000) % 32)
#define BITMAP_BIT_MASK(frame) (1 << BITMAP_BIT_NUMBER(frame))

#define FRAME(idx, bit) (((pmm_frame_t)0x1000) * ((idx)*32 + (bit)))

extern void *_kernel_end;

static uint32_t *bitmap = 0;
static uint32_t bitmap_size = 0;
static uint32_t first_free_index = 0xFFFFFFFF;
static uint8_t  init = 0;

static spinlock_t lock = 0;

static uint32_t __allocated = 0;
static uint32_t __freed     = 0;
static uint32_t __total     = 0;

void k_mem_pmm_init(multiboot_info_t *mb) {
    bitmap = (uint32_t *)(ALIGN((uint32_t)&_kernel_end, 0x1000) + 0x100000);
    k_debug("PMM bitmap at 0x%.8x", bitmap);
    k_info("Memory map:");
    for (uint32_t i = 0; i < mb->mmap_length; i += sizeof(multiboot_memory_map_t)) {
        multiboot_memory_map_t *mmap = (multiboot_memory_map_t *)(mb->mmap_addr + VIRTUAL_BASE + i);
        if (mmap->type == MULTIBOOT_MEMORY_AVAILABLE) {
            k_info("   0x%.9llx - 0x%.9llx Type: 0x%x <-- Usable", mmap->addr,
                   mmap->addr + mmap->len, mmap->type);
            for (uint32_t i = 0; i < mmap->len; i += 0x1000) {
                pmm_frame_t frame = mmap->addr + i;
                if (frame >= (uint32_t)bitmap - VIRTUAL_BASE) {
                    k_mem_pmm_mark_frame(frame);
                }
            }
        } else if (mmap->type == MULTIBOOT_MEMORY_ACPI_RECLAIMABLE) {
            k_info("   0x%.9llx - 0x%.9llx Type: 0x%x <-- ACPI", mmap->addr,
                   mmap->addr + mmap->len, mmap->type);
            k_dev_acpi_set_addr((void*) (mmap->addr + VIRTUAL_BASE));
        } else {
            k_info("   0x%.9llx - 0x%.9llx Type: 0x%x", mmap->addr, mmap->addr + mmap->len, mmap->type);
        }
    }
	
	// Mark all frames that need for us as occupied
	for(uint32_t* i = bitmap; i <= (bitmap + k_mem_pmm_bitmap_size()); i++) {
		uint32_t frame = ((uint32_t) i - VIRTUAL_BASE);
		bitmap[BITMAP_INDEX(frame)] &= ~BITMAP_BIT_MASK(frame);
	}

    init = 1;
	__total     = __freed;
	__freed 	= 0;
	__allocated = 0;
    k_debug("Final bitmap size: %d bytes", k_mem_pmm_bitmap_size() * 4);
}

void k_mem_pmm_set_allocated(pmm_frame_t frame) {
	bitmap[BITMAP_INDEX(frame)] &= ~BITMAP_BIT_MASK(frame);
}

pmm_frame_t k_mem_pmm_alloc_frames(uint32_t frames) {
	LOCK(lock)

	if(!frames) {
		k_panic("Tried to allocate invalid amount of frames.", 0);
	}

    if (first_free_index >= bitmap_size) {
		char buffer[256];
		snprintf(buffer, 256, "Out of physical memory. Tried to allocate %d frames. %d/%d", frames, __allocated - __freed, __total);
		k_panic(buffer, 0);
    }

    uint32_t found_frames = 0;

    uint32_t frame = 0;
    uint8_t  found = 0;

    for (uint32_t i = first_free_index; i < bitmap_size; i++) {
        for (int j = 0; j < 32; j++) {
            if (bitmap[i] & (1 << j)) {
                if (!found_frames) {
                    frame = FRAME(i, j);
                }
                found_frames++;
            } else {
                found_frames = 0;
            }
            if (found_frames >= frames) {
                found = 1;
                break;
            }
        }
        if (found) {
            break;
        }
    }


    if (found) {
        for (uint32_t i = 0; i < frames; i++) {
            uint32_t target_frame = frame + i * 0x1000;
            bitmap[BITMAP_INDEX(target_frame)] &= ~BITMAP_BIT_MASK(target_frame);
        }

        while (first_free_index < bitmap_size && !bitmap[first_free_index]) {
            first_free_index++;
        }
		
		if(first_free_index >= bitmap_size) {
			first_free_index = 0;
		}

        while (first_free_index < bitmap_size && !bitmap[first_free_index]) {
            first_free_index++;
        }

		UNLOCK(lock)

		__allocated += frames;
        return frame;
    }


	char buffer[256];
	snprintf(buffer, 256, "Out of physical memory. Tried to allocate %d frames. %d/%d", frames, __allocated - __freed, __total);
	k_panic(buffer, 0);
	__builtin_unreachable();
}

void k_mem_pmm_free(pmm_frame_t frame, uint32_t size) {
	if(frame % 0x1000) {
		k_warn("Invalid frame alignment: 0x%.8x", frame);
		return;
	}
	LOCK(lock)
    for (pmm_frame_t f = frame; f < frame + ((pmm_frame_t)0x1000) * size; f += 0x1000) {
        uint32_t index = BITMAP_INDEX(f);
        while (index >= bitmap_size) {
            bitmap[bitmap_size] = 0;
            bitmap_size++;
        }
		if(!k_mem_pmm_is_allocated(f)) {
			k_warn("Trying to free non-allocated frame 0x%.8x", f);
			break;
		}
        bitmap[index] |= BITMAP_BIT_MASK(f);
		__freed++;
        if (index < first_free_index) {
            first_free_index = index;
        }
    }
	UNLOCK(lock)
}

uint8_t k_mem_pmm_is_allocated(pmm_frame_t frame) {
    uint32_t index = BITMAP_INDEX(frame);
    if (index >= bitmap_size) {
        return 0;
    }
    return !(bitmap[index] & BITMAP_BIT_MASK(frame));
}

void k_mem_pmm_mark_frame(pmm_frame_t frame) {
    k_mem_pmm_free(frame, 1);
}

uint32_t k_mem_pmm_bitmap_size() {
    return bitmap_size;
}

uint32_t k_mem_pmm_bitmap_end() {
    return (uint32_t)bitmap + k_mem_pmm_bitmap_size() * 4;
}

uint8_t k_mem_pmm_initialized() {
    return init;
}

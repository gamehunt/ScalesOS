#ifndef __PMM_H
#define __PMM_H

#include <multiboot.h>
#include <stdint.h>

typedef uint64_t pmm_frame_t;

void        k_mem_pmm_init(multiboot_info_t* mb);
pmm_frame_t k_mem_pmm_alloc_frame();
void        k_mem_pmm_free(pmm_frame_t frame, uint32_t size);
void        k_mem_pmm_mark_frame(pmm_frame_t frame);
uint8_t     k_mem_pmm_is_allocated(pmm_frame_t frame);
uint32_t    k_mem_pmm_bitmap_size();

#endif
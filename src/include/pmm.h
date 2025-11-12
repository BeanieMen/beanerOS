#ifndef PMM_H
#define PMM_H

#include <stdint.h>
#include <stddef.h>

#define PAGE_SIZE 4096
#define FRAME_SIZE PAGE_SIZE

void pmm_init(uint32_t mem_low, uint32_t mem_high);
void* pmm_alloc_frame(void);
void pmm_free_frame(void* addr);
uint32_t pmm_get_total_frames(void);
uint32_t pmm_get_free_frames(void);

#endif /* PMM_H */

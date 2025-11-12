#include "vmm.h"
#include "pmm.h"
#include <string.h>

#define PAGE_DIRECTORY_INDEX(x) (((x) >> 22) & 0x3FF)
#define PAGE_TABLE_INDEX(x) (((x) >> 12) & 0x3FF)
#define PAGE_ALIGN(x) ((x) & 0xFFFFF000)

static uint32_t kernel_page_directory[1024] __attribute__((aligned(4096)));
static uint32_t kernel_page_tables[256][1024] __attribute__((aligned(4096)));

extern void enable_paging(uint32_t* page_directory);

void vmm_init(void) {
    memset(kernel_page_directory, 0, sizeof(kernel_page_directory));
    memset(kernel_page_tables, 0, sizeof(kernel_page_tables));
    
    for (uint32_t i = 0; i < 256; i++) {
        kernel_page_directory[i] = ((uint32_t)kernel_page_tables[i]) | PAGE_PRESENT | PAGE_RW;
    }
    
    for (uint32_t i = 0; i < 256 * 1024 * 1024; i += PAGE_SIZE) {
        uint32_t pd_index = PAGE_DIRECTORY_INDEX(i);
        uint32_t pt_index = PAGE_TABLE_INDEX(i);
        kernel_page_tables[pd_index][pt_index] = i | PAGE_PRESENT | PAGE_RW;
    }
    
    enable_paging(kernel_page_directory);
}

void vmm_map(page_directory_t dir, uint32_t virt, uint32_t phys, uint32_t flags) {
    uint32_t pd_index = PAGE_DIRECTORY_INDEX(virt);
    uint32_t pt_index = PAGE_TABLE_INDEX(virt);
    
    uint32_t* page_table = (uint32_t*)(dir[pd_index] & 0xFFFFF000);
    
    if (!(dir[pd_index] & PAGE_PRESENT)) {
        page_table = (uint32_t*)pmm_alloc_frame();
        memset(page_table, 0, PAGE_SIZE);
        dir[pd_index] = ((uint32_t)page_table) | PAGE_PRESENT | PAGE_RW | flags;
    }
    
    page_table[pt_index] = PAGE_ALIGN(phys) | flags;
}

void vmm_unmap(page_directory_t dir, uint32_t virt) {
    uint32_t pd_index = PAGE_DIRECTORY_INDEX(virt);
    uint32_t pt_index = PAGE_TABLE_INDEX(virt);
    
    if (!(dir[pd_index] & PAGE_PRESENT)) {
        return;
    }
    
    uint32_t* page_table = (uint32_t*)(dir[pd_index] & 0xFFFFF000);
    page_table[pt_index] = 0;
    
    __asm__ __volatile__("invlpg (%0)" :: "r"(virt));
}

void vmm_switch_directory(page_directory_t dir) {
    __asm__ __volatile__("mov %0, %%cr3" :: "r"(dir));
}

page_directory_t vmm_get_kernel_directory(void) {
    return kernel_page_directory;
}

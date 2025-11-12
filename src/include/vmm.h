#ifndef VMM_H
#define VMM_H

#include <stdint.h>
#include <stddef.h>

#define PAGE_PRESENT    0x1
#define PAGE_RW         0x2
#define PAGE_USER       0x4
#define PAGE_SIZE       4096

typedef uint32_t* page_directory_t;

void vmm_init(void);
void vmm_map(page_directory_t dir, uint32_t virt, uint32_t phys, uint32_t flags);
void vmm_unmap(page_directory_t dir, uint32_t virt);
void vmm_switch_directory(page_directory_t dir);
page_directory_t vmm_get_kernel_directory(void);

#endif /* VMM_H */

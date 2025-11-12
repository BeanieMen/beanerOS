#include <string.h>
#include <stdio.h>
#include "initrd.h"

static initrd_t global_initrd;

initrd_t* initrd_init(uint32_t location) {
    global_initrd.start_address = location;
    
    uint32_t* nfiles_ptr = (uint32_t*)location;
    global_initrd.nfiles = *nfiles_ptr;
    
    global_initrd.headers = (initrd_header_t*)(location + sizeof(uint32_t));
    
    printf("[INITRD] Found at 0x%x\n", location);
    printf("[INITRD] %d files loaded:\n", global_initrd.nfiles);
    
    for (uint32_t i = 0; i < global_initrd.nfiles; i++) {
        if (global_initrd.headers[i].magic == INITRD_MAGIC) {
            printf("  [%d] %s (%d bytes)\n", i, global_initrd.headers[i].name, global_initrd.headers[i].length);
        }
    }
    
    return &global_initrd;
}

uint32_t initrd_get_file_count(initrd_t* initrd) {
    if (!initrd) return 0;
    return initrd->nfiles;
}

const char* initrd_get_file_name(initrd_t* initrd, uint32_t index) {
    if (!initrd || index >= initrd->nfiles) {
        return NULL;
    }
    return initrd->headers[index].name;
}

uint32_t initrd_get_file_size(initrd_t* initrd, uint32_t index) {
    if (!initrd || index >= initrd->nfiles) {
        return 0;
    }
    return initrd->headers[index].length;
}

uint8_t* initrd_read_file(initrd_t* initrd, uint32_t index) {
    if (!initrd || index >= initrd->nfiles) {
        return NULL;
    }
    
    if (initrd->headers[index].magic != INITRD_MAGIC) {
        return NULL;
    }
    
    return (uint8_t*)(initrd->start_address + initrd->headers[index].offset);
}

int initrd_find_file(initrd_t* initrd, const char* name) {
    if (!initrd) return -1;
    
    for (uint32_t i = 0; i < initrd->nfiles; i++) {
        if (strcmp(initrd->headers[i].name, name) == 0) {
            return (int)i;
        }
    }
    return -1;
}

#ifndef INITRD_H
#define INITRD_H

#include <stdint.h>

#define INITRD_MAGIC 0xBF

typedef struct __attribute__((packed)) {
    uint8_t magic;
    char name[64];
    uint32_t offset;
    uint32_t length;
} initrd_header_t;

typedef struct {
    initrd_header_t* headers;
    uint32_t nfiles;
    uint32_t start_address;
} initrd_t;

initrd_t* initrd_init(uint32_t location);
uint32_t initrd_get_file_count(initrd_t* initrd);
const char* initrd_get_file_name(initrd_t* initrd, uint32_t index);
uint32_t initrd_get_file_size(initrd_t* initrd, uint32_t index);
uint8_t* initrd_read_file(initrd_t* initrd, uint32_t index);
int initrd_find_file(initrd_t* initrd, const char* name);

#endif /* INITRD_H */

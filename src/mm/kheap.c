#include "kheap.h"
#include <stdint.h>
#include <string.h>

#define HEAP_START 0x00200000
#define HEAP_SIZE  0x00100000

typedef struct heap_block {
    size_t size;
    int is_free;
    struct heap_block* next;
} heap_block_t;

static heap_block_t* heap_head = NULL;
static uint8_t* heap_memory = (uint8_t*)HEAP_START;

void kheap_init(void) {
    heap_head = (heap_block_t*)heap_memory;
    heap_head->size = HEAP_SIZE - sizeof(heap_block_t);
    heap_head->is_free = 1;
    heap_head->next = NULL;
}

void* kmalloc(size_t size) {
    if (size == 0) {
        return NULL;
    }
    
    size = (size + 7) & ~7;
    
    heap_block_t* current = heap_head;
    
    while (current != NULL) {
        if (current->is_free && current->size >= size) {
            if (current->size >= size + sizeof(heap_block_t) + 8) {
                heap_block_t* new_block = (heap_block_t*)((uint8_t*)current + sizeof(heap_block_t) + size);
                new_block->size = current->size - size - sizeof(heap_block_t);
                new_block->is_free = 1;
                new_block->next = current->next;
                
                current->size = size;
                current->next = new_block;
            }
            
            current->is_free = 0;
            return (void*)((uint8_t*)current + sizeof(heap_block_t));
        }
        current = current->next;
    }
    
    return NULL;
}

void kfree(void* ptr) {
    if (ptr == NULL) {
        return;
    }
    
    heap_block_t* block = (heap_block_t*)((uint8_t*)ptr - sizeof(heap_block_t));
    block->is_free = 1;
    
    heap_block_t* current = heap_head;
    while (current != NULL) {
        if (current->is_free && current->next != NULL && current->next->is_free) {
            current->size += sizeof(heap_block_t) + current->next->size;
            current->next = current->next->next;
        } else {
            current = current->next;
        }
    }
}

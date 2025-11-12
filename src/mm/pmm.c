#include "pmm.h"
#include <string.h>

#define BITMAP_SIZE 32768
#define BITS_PER_BYTE 8

static uint8_t frame_bitmap[BITMAP_SIZE];
static uint32_t total_frames;
static uint32_t free_frames;
static uint32_t first_frame;

static void set_frame(uint32_t frame_addr) {
    uint32_t frame = frame_addr / FRAME_SIZE;
    uint32_t idx = frame / BITS_PER_BYTE;
    uint32_t off = frame % BITS_PER_BYTE;
    frame_bitmap[idx] |= (1 << off);
}

static void clear_frame(uint32_t frame_addr) {
    uint32_t frame = frame_addr / FRAME_SIZE;
    uint32_t idx = frame / BITS_PER_BYTE;
    uint32_t off = frame % BITS_PER_BYTE;
    frame_bitmap[idx] &= ~(1 << off);
}

static int test_frame(uint32_t frame_addr) {
    uint32_t frame = frame_addr / FRAME_SIZE;
    uint32_t idx = frame / BITS_PER_BYTE;
    uint32_t off = frame % BITS_PER_BYTE;
    return (frame_bitmap[idx] & (1 << off));
}

static uint32_t find_free_frame(void) {
    for (uint32_t i = 0; i < total_frames; i++) {
        uint32_t frame_addr = (first_frame + i) * FRAME_SIZE;
        if (!test_frame(frame_addr)) {
            return frame_addr;
        }
    }
    return 0;
}

void pmm_init(uint32_t mem_low, uint32_t mem_high) {
    memset(frame_bitmap, 0, BITMAP_SIZE);
    
    first_frame = (mem_low + FRAME_SIZE - 1) / FRAME_SIZE;
    total_frames = (mem_high / FRAME_SIZE) - first_frame;
    
    if (total_frames > BITMAP_SIZE * BITS_PER_BYTE) {
        total_frames = BITMAP_SIZE * BITS_PER_BYTE;
    }
    
    free_frames = total_frames;
    
    for (uint32_t i = 0; i < mem_low; i += FRAME_SIZE) {
        set_frame(i);
    }
}

void* pmm_alloc_frame(void) {
    if (free_frames == 0) {
        return NULL;
    }
    
    uint32_t frame_addr = find_free_frame();
    if (frame_addr == 0) {
        return NULL;
    }
    
    set_frame(frame_addr);
    free_frames--;
    
    return (void*)frame_addr;
}

void pmm_free_frame(void* addr) {
    uint32_t frame_addr = (uint32_t)addr;
    
    if (!test_frame(frame_addr)) {
        return;
    }
    
    clear_frame(frame_addr);
    free_frames++;
}

uint32_t pmm_get_total_frames(void) {
    return total_frames;
}

uint32_t pmm_get_free_frames(void) {
    return free_frames;
}

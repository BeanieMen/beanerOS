#include "tty.h"
#include "io.h"
#include "serial.h"
#include <string.h>

#define VGA_ADDRESS 0xB8000
#define VGA_WIDTH 80
#define VGA_HEIGHT 25

static unsigned int terminal_row;
static unsigned int terminal_column;
static unsigned char terminal_color;
static unsigned short* terminal_buffer;

static inline unsigned char vga_entry_color(enum vga_color fg, enum vga_color bg) {
    return fg | bg << 4;
}

static inline unsigned short vga_entry(unsigned char uc, unsigned char color) {
    return (unsigned short) uc | (unsigned short) color << 8;
}

static void update_cursor(unsigned int x, unsigned int y) {
    unsigned short pos = y * VGA_WIDTH + x;
    
    outb(0x3D4, 0x0F);
    outb(0x3D5, (unsigned char)(pos & 0xFF));
    outb(0x3D4, 0x0E);
    outb(0x3D5, (unsigned char)((pos >> 8) & 0xFF));
}

static void terminal_scroll(void) {
    for (unsigned int y = 0; y < VGA_HEIGHT - 1; y++) {
        for (unsigned int x = 0; x < VGA_WIDTH; x++) {
            unsigned int dest = y * VGA_WIDTH + x;
            unsigned int src = (y + 1) * VGA_WIDTH + x;
            terminal_buffer[dest] = terminal_buffer[src];
        }
    }
    
    for (unsigned int x = 0; x < VGA_WIDTH; x++) {
        unsigned int index = (VGA_HEIGHT - 1) * VGA_WIDTH + x;
        terminal_buffer[index] = vga_entry(' ', terminal_color);
    }
    
    terminal_row = VGA_HEIGHT - 1;
}

static void terminal_putentryat(char c, unsigned char color, unsigned int x, unsigned int y) {
    const unsigned int index = y * VGA_WIDTH + x;
    terminal_buffer[index] = vga_entry(c, color);
    serial_putchar(c);
}

void terminal_initialize(void) {
    terminal_row = 0;
    terminal_column = 0;
    terminal_color = vga_entry_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
    terminal_buffer = (unsigned short*) VGA_ADDRESS;
    
    for (unsigned int y = 0; y < VGA_HEIGHT; y++) {
        for (unsigned int x = 0; x < VGA_WIDTH; x++) {
            const unsigned int index = y * VGA_WIDTH + x;
            terminal_buffer[index] = vga_entry(' ', terminal_color);
        }
    }
}

void terminal_clear(void) {
    terminal_row = 0;
    terminal_column = 0;
    
    for (unsigned int y = 0; y < VGA_HEIGHT; y++) {
        for (unsigned int x = 0; x < VGA_WIDTH; x++) {
            const unsigned int index = y * VGA_WIDTH + x;
            terminal_buffer[index] = vga_entry(' ', terminal_color);
        }
    }
    
    update_cursor(terminal_column, terminal_row);
}

void terminal_enable_cursor(void) {
    outb(0x3D4, 0x0A);
    outb(0x3D5, (inb(0x3D5) & 0xC0) | 0);
    outb(0x3D4, 0x0B);
    outb(0x3D5, (inb(0x3D5) & 0xE0) | 15);
}

void terminal_setcolor(unsigned char color) {
    terminal_color = color;
}

void terminal_set_position(unsigned int x, unsigned int y) {
    terminal_column = x;
    terminal_row = y;
    update_cursor(terminal_column, terminal_row);
}

unsigned int terminal_get_column(void) {
    return terminal_column;
}

unsigned int terminal_get_row(void) {
    return terminal_row;
}

void terminal_putchar(char c) {
    if (c == '\n') {
        terminal_column = 0;
        if (++terminal_row == VGA_HEIGHT) {
            terminal_scroll();
        }
        update_cursor(terminal_column, terminal_row);
        serial_putchar('\n');
        return;
    }
    
    terminal_putentryat(c, terminal_color, terminal_column, terminal_row);
    if (++terminal_column == VGA_WIDTH) {
        terminal_column = 0;
        if (++terminal_row == VGA_HEIGHT) {
            terminal_scroll();
        }
    }
    update_cursor(terminal_column, terminal_row);
}

void terminal_putchar_color(char c, enum vga_color fg, enum vga_color bg) {
    unsigned char old_color = terminal_color;
    terminal_color = vga_entry_color(fg, bg);
    terminal_putchar(c);
    terminal_color = old_color;
}

void terminal_write(const char* data, unsigned int size) {
    for (unsigned int i = 0; i < size; i++)
        terminal_putchar(data[i]);
}

void terminal_writestring(const char* data) {
    terminal_write(data, strlen(data));
}

void terminal_writestring_color(const char* data, enum vga_color fg, enum vga_color bg) {
    unsigned char old_color = terminal_color;
    terminal_color = vga_entry_color(fg, bg);
    terminal_writestring(data);
    terminal_color = old_color;
}

void terminal_backspace(void) {
    terminal_column--;
    terminal_putentryat(' ', terminal_color, terminal_column, terminal_row);
    update_cursor(terminal_column, terminal_row);
}

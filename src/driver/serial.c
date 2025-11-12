#include "serial.h"
#include "io.h"

#define SERIAL_COM1 0x3F8

static int is_transmit_empty(void) {
    return inb(SERIAL_COM1 + 5) & 0x20;
}

void serial_init(void) {
    outb(SERIAL_COM1 + 1, 0x00);
    outb(SERIAL_COM1 + 3, 0x80);
    outb(SERIAL_COM1 + 0, 0x03);
    outb(SERIAL_COM1 + 1, 0x00);
    outb(SERIAL_COM1 + 3, 0x03);
    outb(SERIAL_COM1 + 2, 0xC7);
    outb(SERIAL_COM1 + 4, 0x0B);
}

void serial_putchar(char c) {
    while (is_transmit_empty() == 0);
    outb(SERIAL_COM1, c);
}

void serial_write(const char* data) {
    while (*data) {
        serial_putchar(*data++);
    }
}

void serial_write_buf(const char* data, unsigned int size) {
    for (unsigned int i = 0; i < size; i++) {
        serial_putchar(data[i]);
    }
}

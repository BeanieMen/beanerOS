#include "keyboard.h"
#include "io.h"
#include "idt.h"

#define KEYBOARD_BUFFER_SIZE 256

static const char scancode_to_ascii[] = {
    0, 0, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',
    '\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',
    0, 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`',
    0, '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0,
    '*', 0, ' '
};

static const char scancode_to_ascii_shifted[] = {
    0, 0, '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+', '\b',
    '\t', 'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', '\n',
    0, 'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '"', '~',
    0, '|', 'Z', 'X', 'C', 'V', 'B', 'N', 'M', '<', '>', '?', 0,
    '*', 0, ' '
};

static char keyboard_buffer[KEYBOARD_BUFFER_SIZE];
static volatile unsigned int buffer_read = 0;
static volatile unsigned int buffer_write = 0;
static int shift_pressed = 0;

static void keyboard_irq_handler(registers_t* regs) {
    (void)regs;
    unsigned char scancode = inb(0x60);
    
    if (scancode == 0x2A || scancode == 0x36) {
        shift_pressed = 1;
    } else if (scancode == 0xAA || scancode == 0xB6) {
        shift_pressed = 0;
    } else if (!(scancode & 0x80)) {
        if (scancode < sizeof(scancode_to_ascii)) {
            char ascii = shift_pressed ? scancode_to_ascii_shifted[scancode] : scancode_to_ascii[scancode];
            if (ascii != 0) {
                unsigned int next_write = (buffer_write + 1) % KEYBOARD_BUFFER_SIZE;
                if (next_write != buffer_read) {
                    keyboard_buffer[buffer_write] = ascii;
                    buffer_write = next_write;
                }
            }
        }
    }
}

void keyboard_init_irq(void) {
    buffer_read = 0;
    buffer_write = 0;
    register_interrupt_handler(33, &keyboard_irq_handler);
}

char keyboard_getchar(void) {
    while (buffer_read == buffer_write) {
        __asm__ __volatile__("hlt");
    }
    
    char c = keyboard_buffer[buffer_read];
    buffer_read = (buffer_read + 1) % KEYBOARD_BUFFER_SIZE;
    return c;
}

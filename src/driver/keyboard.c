#include "keyboard.h"
#include "io.h"
static const char scancode_to_ascii[] = {
    0, 0, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',
    '\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',
    0, 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`',
    0, '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0,
    '*', 0, ' '
};
char keyboard_getchar(void) {
    unsigned char scancode;
    while (!(inb(0x64) & 0x01)) {
        __asm__ __volatile__("pause");
    }
    scancode = inb(0x60);
    if (scancode & 0x80) {
        return 0;
    }
    if (scancode < sizeof(scancode_to_ascii)) {
        return scancode_to_ascii[scancode];
    }
    return 0;
}

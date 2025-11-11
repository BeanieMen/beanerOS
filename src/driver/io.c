#include "io.h"
void delay(unsigned int count) {
    for (unsigned int i = 0; i < count; i++) {
        __asm__ __volatile__("nop");
    }
}

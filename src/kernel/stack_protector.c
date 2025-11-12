#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

uintptr_t __stack_chk_guard = 0xe2dee396U;

void __stack_chk_fail(void) {
    printf("STACK SMASHING DETECTED!\n");
    abort();
}

void __stack_chk_fail_local(void) {
    __stack_chk_fail();
}

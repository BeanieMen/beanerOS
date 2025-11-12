#include <stdio.h>
#include <stdlib.h>

__attribute__((__noreturn__))
void abort(void) {
#if defined(__is_libk)
	printf("KERNEL PANIC: abort() called\n");
	printf("System halted.\n");
#else
	printf("abort()\n");
#endif
	while (1) { 
		__asm__ __volatile__("hlt");
	}
	__builtin_unreachable();
}

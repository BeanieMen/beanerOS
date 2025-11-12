#include "tty.h"
#include "keyboard.h"
#include "io.h"
#include "pmm.h"
#include "vmm.h"
#include "kheap.h"
#include "idt.h"
#include "timer.h"
#include "serial.h"
#include "scheduler.h"
#include "initrd.h"
#include <stdio.h>
#include <string.h>

typedef struct multiboot_info {
    uint32_t flags;
    uint32_t mem_lower;
    uint32_t mem_upper;
    uint32_t boot_device;
    uint32_t cmdline;
    uint32_t mods_count;
    uint32_t mods_addr;
    uint32_t syms[4];
    uint32_t mmap_length;
    uint32_t mmap_addr;
} multiboot_info_t;

typedef struct multiboot_mod {
    uint32_t mod_start;
    uint32_t mod_end;
    uint32_t string;
    uint32_t reserved;
} multiboot_mod_t;

static void print_status(const char* message) {
    terminal_writestring_color("[OK]", VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
    terminal_writestring(" ");
    terminal_writestring(message);
    terminal_writestring("\n");
}

static void draw_banner(void) {
    const char* banner[] = {
        "  ____                              ",
        " | __ )  ___  __ _ _ __   ___ _ __ ",
        " |  _ \\ / _ \\/ _` | '_ \\ / _ \\ '__|",
        " | |_) |  __/ (_| | | | |  __/ |   ",
        " |____/ \\___|\\__,_|_| |_|\\___|_|   ",
    };

    enum vga_color colors[] = {
        VGA_COLOR_LIGHT_RED,
        VGA_COLOR_LIGHT_MAGENTA,
        VGA_COLOR_LIGHT_BLUE,
        VGA_COLOR_LIGHT_CYAN,
        VGA_COLOR_LIGHT_GREEN
    };

    terminal_writestring("\n");
    for (unsigned int i = 0; i < 5; i++) {
        terminal_writestring_color(banner[i], colors[i], VGA_COLOR_BLACK);
        terminal_writestring("\n");
    }
    terminal_writestring("\n");
}

static void spinning_animation(void) {
    const char spinner[] = {'|', '/', '-', '\\'};
    unsigned int saved_col = terminal_get_column();
    unsigned int saved_row = terminal_get_row();

    terminal_writestring_color("Booting", VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK);

    for (unsigned int i = 0; i < 20; i++) {
        terminal_putchar(' ');
        terminal_putchar(spinner[i % 4]);
        delay(5000000);
        terminal_set_position(saved_col + 7, saved_row);
    }
    terminal_set_position(saved_col + 7, saved_row);
    terminal_writestring("  Done!\n\n");
}

static initrd_t *g_initrd = NULL;

static void cmd_ls(void) {
    if (!g_initrd) {
        printf("No initrd loaded\n");
        return;
    }

    uint32_t count = initrd_get_file_count(g_initrd);
    printf("Files in initrd: %d\n", count);
    
    for (uint32_t i = 0; i < count; i++) {
        const char* name = initrd_get_file_name(g_initrd, i);
        uint32_t size = initrd_get_file_size(g_initrd, i);
        printf("  %s (%d bytes)\n", name, size);
    }
}

static void cmd_cat(const char* filename) {
    if (!g_initrd) {
        printf("No initrd loaded\n");
        return;
    }

    int index = initrd_find_file(g_initrd, filename);
    if (index < 0) {
        printf("File not found: %s\n", filename);
        return;
    }

    uint8_t* data = initrd_read_file(g_initrd, (uint32_t)index);
    uint32_t size = initrd_get_file_size(g_initrd, (uint32_t)index);

    if (!data) {
        printf("Error reading file\n");
        return;
    }

    for (uint32_t i = 0; i < size; i++) {
        char c = (char)data[i];
        if (c >= 32 && c <= 126) {
            terminal_putchar(c);
        } else if (c == '\n' || c == '\t') {
            terminal_putchar(c);
        }
    }
    terminal_putchar('\n');
}

static void process_command(const char* cmd) {
    if (cmd[0] == '\0') {
        return;
    }

    if (strcmp(cmd, "ls") == 0) {
        cmd_ls();
    } else if (strcmp(cmd, "help") == 0) {
        printf("Available commands:\n");
        printf("  ls           - List files in initrd\n");
        printf("  cat <file>   - Display file contents\n");
        printf("  help         - Show this help\n");
        printf("  clear        - Clear the screen\n");
    } else if (strcmp(cmd, "clear") == 0) {
        terminal_clear();
    } else if (strncmp(cmd, "cat ", 4) == 0) {
        cmd_cat(cmd + 4);
    } else {
        printf("Unknown command: %s\n", cmd);
        printf("Type 'help' for available commands\n");
    }
}

static void shell_loop(void) {
    char cmdbuf[256];
    unsigned int cmdlen = 0;

    terminal_writestring_color("Welcome to Beaner OS!\n", VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK);
    terminal_writestring("Type 'help' for available commands\n");
    terminal_writestring("> ");

    while (1) {
        char c = keyboard_getchar();

        if (c == 0)
            continue;

        if (c == '\b') {
            if (terminal_get_column() > 2 && cmdlen > 0) {
                terminal_backspace();
                cmdlen--;
            }
            continue;
        }

        if (c == '\n') {
            terminal_putchar('\n');
            cmdbuf[cmdlen] = '\0';
            process_command(cmdbuf);
            cmdlen = 0;
            terminal_writestring("> ");
            continue;
        }

        if (cmdlen < sizeof(cmdbuf) - 1) {
            terminal_putchar(c);
            cmdbuf[cmdlen++] = c;
        }
    }
}

void kmain(multiboot_info_t *mboot_info) {
    terminal_initialize();
    terminal_enable_cursor();

    spinning_animation();
    draw_banner();

    printf("Welcome to Beaner!\n\n");

    print_status("Terminal initialized");
    
    serial_init();
    serial_write("Beaner kernel starting...\n");
    print_status("Serial driver initialized");
    
    idt_init();
    print_status("IDT initialized");
    
    __asm__ __volatile__("sti");
    print_status("Interrupts enabled");
    
    timer_init(100);
    print_status("Timer initialized (100Hz)");
    
    keyboard_init_irq();
    print_status("Keyboard IRQ handler registered");
    
    pmm_init(0x00400000, 0x02000000);
    printf("[OK] PMM initialized (%d frames available)\n", pmm_get_free_frames());
    
    kheap_init();
    print_status("Kernel heap initialized");
    
    scheduler_init();
    print_status("Scheduler initialized");

    if (mboot_info && (mboot_info->flags & 0x8) && mboot_info->mods_count > 0) {
        multiboot_mod_t *mod = (multiboot_mod_t *)mboot_info->mods_addr;
        g_initrd = initrd_init(mod->mod_start);
        if (g_initrd) {
            printf("[OK] Initrd loaded (%d files)\n", initrd_get_file_count(g_initrd));
        }
    }
    
    printf("\n");
    print_status("All systems operational");
    printf("\n");

    shell_loop();
}

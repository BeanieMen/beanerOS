#include "tty.h"
#include "keyboard.h"
#include "io.h"
#include "pmm.h"
#include "kheap.h"
#include "idt.h"
#include "timer.h"
#include "serial.h"
#include "ata.h"
#include "fat32.h"
#include "shell.h"
#include <stdio.h>

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

static void print_ok(const char *msg) {
    terminal_writestring_color("[OK]", VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
    printf(" %s\n", msg);
}

static void draw_banner(void) {
    const char *banner[] = {
        "  ____                              ",
        " | __ )  ___  __ _ _ __   ___ _ __ ",
        " |  _ \\ / _ \\/ _` | '_ \\ / _ \\ '__|",
        " | |_) |  __/ (_| | | | |  __/ |   ",
        " |____/ \\___|\\__,_|_| |_|\\___|_|   ",
    };

    enum vga_color colors[] = {
        VGA_COLOR_LIGHT_RED, VGA_COLOR_LIGHT_MAGENTA,
        VGA_COLOR_LIGHT_BLUE, VGA_COLOR_LIGHT_CYAN,
        VGA_COLOR_LIGHT_GREEN
    };

    terminal_writestring("\n");
    for (int i = 0; i < 5; i++) {
        terminal_writestring_color(banner[i], colors[i], VGA_COLOR_BLACK);
        terminal_writestring("\n");
    }
    terminal_writestring("\n");
}

static void boot_animation(void) {
    const char spinner[] = {'|', '/', '-', '\\'};
    unsigned int col = terminal_get_column();
    unsigned int row = terminal_get_row();

    terminal_writestring_color("Booting", VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK);

    for (int i = 0; i < 20; i++) {
        terminal_putchar(' ');
        terminal_putchar(spinner[i % 4]);
        delay(5000000);
        terminal_set_position(col + 7, row);
    }
    terminal_set_position(col + 7, row);
    terminal_writestring("  Done!\n\n");
}

fat32_fs_t g_fs;

void kmain(multiboot_info_t *mboot_info) {
    (void)mboot_info;
    
    terminal_initialize();
    terminal_enable_cursor();
    boot_animation();
    draw_banner();

    printf("Welcome to Beaner!\n\n");

    print_ok("Terminal initialized");
    
    serial_init();
    serial_write("Beaner kernel starting...\n");
    print_ok("Serial driver initialized");
    
    idt_init();
    print_ok("IDT initialized");
    
    __asm__ __volatile__("sti");
    print_ok("Interrupts enabled");
    
    timer_init(100);
    print_ok("Timer initialized (100Hz)");
    
    keyboard_init_irq();
    print_ok("Keyboard IRQ handler registered");
    
    pmm_init(0x00400000, 0x02000000);
    printf("[OK] PMM initialized (%d frames available)\n", pmm_get_free_frames());
    
    kheap_init();
    print_ok("Kernel heap initialized");
    
    ata_init();
    print_ok("ATA driver initialized");
    
    if (fat32_init(&g_fs) == 0) {
        print_ok("FAT32 filesystem mounted");
    } else {
        printf("[WARN] Failed to mount FAT32 filesystem\n");
    }

    printf("\n");
    print_ok("All systems operational");
    printf("\n");

    shell_loop();
}

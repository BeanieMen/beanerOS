#include "vga.h"
#include "keyboard.h"
#include "io.h"

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

static void shell_loop(void) {
    enum vga_color current_color = VGA_COLOR_WHITE;
    enum vga_color colors_cycle[] = {
        VGA_COLOR_LIGHT_RED, VGA_COLOR_LIGHT_MAGENTA,
        VGA_COLOR_LIGHT_BLUE, VGA_COLOR_LIGHT_CYAN,
        VGA_COLOR_LIGHT_GREEN, VGA_COLOR_LIGHT_BROWN, VGA_COLOR_WHITE
    };
    unsigned int color_index = 0;

    terminal_writestring_color("Type something (press ESC to see colors):\n",
                              VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK);
    terminal_writestring("> ");

    while (1) {
        char c = keyboard_getchar();

        if (c == 0)
            continue;

        if (c == 0x1B || c == 27) {
            color_index = (color_index + 1) % 7;
            current_color = colors_cycle[color_index];
            continue;
        }

        if (c == '\b') {
            if (terminal_get_column() > 2)
                terminal_backspace();
            continue;
        }

        if (c == '\n') {
            terminal_putchar('\n');
            terminal_writestring("> ");
            continue;
        }

        terminal_putchar_color(c, current_color, VGA_COLOR_BLACK);
    }
}

void kmain(void) {
    terminal_initialize();
    terminal_enable_cursor();

    spinning_animation();
    draw_banner();

    terminal_writestring_color("Welcome to BeanerOS!", VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
    terminal_writestring("\n\n");

    print_status("Kernel initialized");
    print_status("VGA text mode ready");
    print_status("Keyboard driver loaded");
    terminal_writestring("\n");

    shell_loop();
}

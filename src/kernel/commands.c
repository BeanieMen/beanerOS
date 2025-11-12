#include "commands.h"
#include "fat32.h"
#include "kheap.h"
#include "tty.h"
#include <stdio.h>
#include <string.h>

void cmd_ls(fat32_fs_t *fs) {
    fat32_list_root(fs);
}

void cmd_cat(fat32_fs_t *fs, const char *filename) {
    uint8_t *data;
    uint32_t size;
    
    if (fat32_read_file(fs, filename, &data, &size) != 0) {
        printf("File not found: %s\n", filename);
        return;
    }
    
    if (data == NULL || size == 0) {
        printf("(empty file)\n");
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
    
    kfree(data);
}

void cmd_echo(fat32_fs_t *fs, const char *text, const char *output_file, int is_append) {
    if (output_file) {
        if (output_file[0] == '\0') {
            printf("Error: No output file specified\n");
            return;
        }
        
        uint32_t len = strlen(text);
        uint8_t *data = (uint8_t*)kmalloc(len + 2);
        if (!data) {
            printf("Failed to allocate memory\n");
            return;
        }
        
        memcpy(data, text, len);
        data[len] = '\n';
        data[len + 1] = '\0';
        
        if (is_append) {
            if (fat32_append_file(fs, output_file, data, len + 1) != 0) {
                printf("Failed to append to: %s\n", output_file);
            }
        } else {
            if (fat32_write_file(fs, output_file, data, len + 1) != 0) {
                printf("Failed to write to: %s\n", output_file);
            }
        }
        kfree(data);
    } else {
        printf("%s\n", text);
    }
}

void cmd_touch(fat32_fs_t *fs, const char *filename) {
    if (fat32_create_file(fs, filename) == 0) {
        printf("Created: %s\n", filename);
    } else {
        printf("Failed to create: %s\n", filename);
    }
}

void cmd_mkdir(fat32_fs_t *fs, const char *dirname) {
    if (fat32_create_directory(fs, dirname) == 0) {
        printf("Created directory: %s\n", dirname);
    } else {
        printf("Failed to create directory: %s\n", dirname);
    }
}

void cmd_cd(fat32_fs_t *fs, const char *dirname) {
    if (fat32_change_directory(fs, dirname) == 0) {
    } else {
        printf("Failed to change directory: %s\n", dirname);
    }
}

void cmd_pwd(fat32_fs_t *fs) {
    printf("%s\n", fs->current_dir);
}

void cmd_help(void) {
    printf("Available commands:\n");
    printf("  ls               - List files\n");
    printf("  cat <file>       - Display file contents\n");
    printf("  echo <text>      - Print text or write to file\n");
    printf("  touch <file>     - Create empty file\n");
    printf("  mkdir <dir>      - Create directory\n");
    printf("  cd <dir>         - Change directory\n");
    printf("  pwd              - Print working directory\n");
    printf("  help             - Show this help\n");
    printf("  clear            - Clear the screen\n");
}

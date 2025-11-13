#include "commands.h"
#include "fat.h"
#include "kheap.h"
#include "tty.h"
#include <stdio.h>
#include <string.h>

static char g_cwd[256] = "/root";
static const size_t VOLUME_PREFIX_LEN = 5;

static void build_path(char *dest, const char *filename, size_t filename_len) {

        size_t cwd_len = strlen(g_cwd);
        memcpy(dest, g_cwd, cwd_len);
        
        if (cwd_len > 0 && g_cwd[cwd_len - 1] != '/') {
            dest[cwd_len++] = '/';
        }
        
        memcpy(dest + cwd_len, filename, filename_len);
        dest[cwd_len + filename_len] = '\0';
}

void cmd_ls(Fat *fs) {
    (void)fs;
    Dir dir;
    DirInfo info;
    
    if (fat_dir_open(&dir, g_cwd) != FAT_ERR_NONE) {
        printf("Failed to open directory\n");
        return;
    }
    
    printf("Directory contents:\n");
    while (fat_dir_read(&dir, &info) == FAT_ERR_NONE) {
        if (info.attr & FAT_ATTR_DIR) {
            printf("  [DIR]  %.*s\n", (int)info.name_len, info.name);
        } else {
            printf("  [FILE] %.*s (%u bytes)\n", (int)info.name_len, info.name, info.size);
        }
        fat_dir_next(&dir);
    }
}

void cmd_cat(Fat *fs, const char *filename) {
    (void)fs;
    if (!filename) {
        printf("Usage: cat <file>\n");
        return;
    }

    while (*filename == ' ' || *filename == '\t') {
        filename++;
    }

    size_t name_len = 0;
    while (name_len < 249 && filename[name_len] && filename[name_len] != '\n') {
        name_len++;
    }
    while (name_len > 0 && (filename[name_len - 1] == ' ' || filename[name_len - 1] == '\t')) {
        name_len--;
    }

    if (name_len == 0) {
        printf("Usage: cat <file>\n");
        return;
    }

    // Build path using static buffer to avoid stack issues
    static char path[256];
    build_path(path, filename, name_len);

    File file;
    int err = fat_file_open(&file, path, FAT_READ);
    if (err != FAT_ERR_NONE) {
        printf("cat: cannot open '%.*s': %s\n", (int)name_len, filename, fat_get_error(err));
        return;
    }

    // Check for empty files
    if (file.size == 0) {
        fat_file_close(&file);
        return;
    }

    // Check for invalid cluster (shouldn't happen with non-empty files)
    if (file.sclust == 0) {
        printf("cat: '%.*s': invalid file cluster\n", (int)name_len, filename);
        fat_file_close(&file);
        return;
    }

    uint8_t buffer[512];

    for (;;) {
        int bytes_read = 0;
        err = fat_file_read(&file, buffer, sizeof(buffer), &bytes_read);

        if (err != FAT_ERR_NONE) {
            printf("cat: read error: %s\n", fat_get_error(err));
            break;
        }

        if (bytes_read == 0) {
            break;
        }

        terminal_write((const char*)buffer, (unsigned int)bytes_read);
    }

    fat_file_close(&file);
}

void cmd_echo(Fat *fs, const char *text, const char *output_file, int is_append) {
    (void)fs;
    if (output_file) {
        if (output_file[0] == '\0') {
            printf("Error: No output file specified\n");
            return;
        }
        
        static char path[256];
        size_t name_len = 0;
        while (name_len < 249 && output_file[name_len]) {
            name_len++;
        }
        build_path(path, output_file, name_len);
        
        File file;
        uint8_t flags = FAT_WRITE | FAT_CREATE;
        if (is_append) {
            flags |= FAT_APPEND;
        } else {
            flags |= FAT_TRUNC;
        }
        
        if (fat_file_open(&file, path, flags) != FAT_ERR_NONE) {
            printf("Failed to open file: %s\n", output_file);
            return;
        }
        
        int bytes_written;
        int len = strlen(text);
        fat_file_write(&file, text, len, &bytes_written);
        fat_file_write(&file, "\n", 1, &bytes_written);
        fat_file_close(&file);
    } else {
        printf("%s\n", text);
    }
}

void cmd_touch(Fat *fs, const char *filename) {
    (void)fs;
    static char path[256];
    size_t name_len = 0;
    while (name_len < 249 && filename[name_len]) {
        name_len++;
    }
    build_path(path, filename, name_len);
    
    File file;
    if (fat_file_open(&file, path, FAT_CREATE | FAT_WRITE) == FAT_ERR_NONE) {
        fat_file_close(&file);
        printf("Created: %s\n", filename);
    } else {
        printf("Failed to create: %s\n", filename);
    }
}

void cmd_mkdir(Fat *fs, const char *dirname) {
    (void)fs;
    static char path[256];
    size_t name_len = 0;
    while (name_len < 249 && dirname[name_len]) {
        name_len++;
    }
    build_path(path, dirname, name_len);
    
    Dir dir;
    if (fat_dir_create(&dir, path) == FAT_ERR_NONE) {
        printf("Created directory: %s\n", dirname);
    } else {
        printf("Failed to create directory: %s\n", dirname);
    }
}

void cmd_cd(Fat *fs, const char *dirname) {
    (void)fs;
    (void)dirname;
    printf("cd not yet implemented\n");
}

void cmd_pwd(Fat *fs) {
    (void)fs;
    if (strlen(g_cwd) > VOLUME_PREFIX_LEN) {
        printf("%s\n", g_cwd + VOLUME_PREFIX_LEN);
    } else {
        printf("/\n");
    }
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

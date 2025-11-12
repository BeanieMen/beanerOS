#include "shell.h"
#include "commands.h"
#include "keyboard.h"
#include "tty.h"
#include "fat32.h"
#include <stdio.h>
#include <string.h>

extern fat32_fs_t g_fs;

static char* trim_whitespace(char *str) {
    while (*str == ' ' || *str == '\t') str++;
    
    char *end = str + strlen(str) - 1;
    while (end > str && (*end == ' ' || *end == '\t' || *end == '\n')) {
        *end = '\0';
        end--;
    }
    
    return str;
}

static void parse_redirection(char *cmd, char **actual_cmd, char **output_file, int *is_append) {
    *output_file = NULL;
    *is_append = 0;
    
    char *append_pos = strstr(cmd, ">>");
    if (append_pos) {
        *append_pos = '\0';
        *output_file = trim_whitespace(append_pos + 2);
        
        size_t len = strlen(*output_file);
        if (len > 0) {
            char *end = *output_file + len - 1;
            while (end >= *output_file && (*end == ' ' || *end == '\n')) {
                *end = '\0';
                end--;
            }
        }
        *is_append = 1;
    } else {
        char *out_pos = strchr(cmd, '>');
        if (out_pos) {
            *out_pos = '\0';
            *output_file = trim_whitespace(out_pos + 1);
            
            size_t len = strlen(*output_file);
            if (len > 0) {
                char *end = *output_file + len - 1;
                while (end >= *output_file && (*end == ' ' || *end == '\n' || *end == '>')) {
                    *end = '\0';
                    end--;
                }
            }
        }
    }
    
    *actual_cmd = trim_whitespace(cmd);
}

static void process_command(const char *cmd) {
    if (cmd[0] == '\0') return;

    char cmd_copy[256];
    strncpy(cmd_copy, cmd, 255);
    cmd_copy[255] = '\0';
    
    char *actual_cmd;
    char *output_file;
    int is_append;
    
    parse_redirection(cmd_copy, &actual_cmd, &output_file, &is_append);

    if (strcmp(actual_cmd, "ls") == 0) {
        cmd_ls(&g_fs);
    } else if (strcmp(actual_cmd, "help") == 0) {
        cmd_help();
    } else if (strcmp(actual_cmd, "clear") == 0) {
        terminal_clear();
    } else if (strcmp(actual_cmd, "pwd") == 0) {
        cmd_pwd(&g_fs);
    } else if (strncmp(actual_cmd, "echo ", 5) == 0) {
        cmd_echo(&g_fs, actual_cmd + 5, output_file, is_append);
    } else if (strncmp(actual_cmd, "touch ", 6) == 0) {
        cmd_touch(&g_fs, actual_cmd + 6);
    } else if (strncmp(actual_cmd, "mkdir ", 6) == 0) {
        cmd_mkdir(&g_fs, actual_cmd + 6);
    } else if (strncmp(actual_cmd, "cd ", 3) == 0) {
        cmd_cd(&g_fs, actual_cmd + 3);
    } else if (strcmp(actual_cmd, "cd") == 0) {
        cmd_cd(&g_fs, "/");
    } else if (strncmp(actual_cmd, "cat ", 4) == 0) {
        cmd_cat(&g_fs, actual_cmd + 4);
    } else {
        printf("Unknown command: %s\n", actual_cmd);
        printf("Type 'help' for available commands\n");
    }
}

void shell_loop(void) {
    char cmdbuf[256];
    unsigned int cmdlen = 0;

    terminal_writestring_color("Welcome to Beaner OS!\n", VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK);
    terminal_writestring("Type 'help' for available commands\n");
    terminal_writestring("> ");

    while (1) {
        char c = keyboard_getchar();

        if (c == 0) continue;

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

#ifndef _COMMANDS_H
#define _COMMANDS_H

#include "fat32.h"

void cmd_ls(fat32_fs_t *fs);
void cmd_cat(fat32_fs_t *fs, const char *filename);
void cmd_echo(fat32_fs_t *fs, const char *text, const char *output_file, int is_append);
void cmd_touch(fat32_fs_t *fs, const char *filename);
void cmd_mkdir(fat32_fs_t *fs, const char *dirname);
void cmd_cd(fat32_fs_t *fs, const char *dirname);
void cmd_pwd(fat32_fs_t *fs);
void cmd_help(void);

#endif

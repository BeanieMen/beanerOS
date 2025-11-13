#ifndef _COMMANDS_H
#define _COMMANDS_H

#include "fat.h"

void cmd_ls(Fat *fs);
void cmd_cat(Fat *fs, const char *filename);
void cmd_echo(Fat *fs, const char *text, const char *output_file, int is_append);
void cmd_touch(Fat *fs, const char *filename);
void cmd_mkdir(Fat *fs, const char *dirname);
void cmd_cd(Fat *fs, const char *dirname);
void cmd_pwd(Fat *fs);
void cmd_help(void);

#endif

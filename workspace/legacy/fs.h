#ifndef FS_H
#define FS_H

#include <stdint.h>

#define MAX_FILES 32
#define MAX_NAME_LEN 32
#define MAX_FILE_DATA 512

struct File {
    char name[MAX_NAME_LEN];
    char data[MAX_FILE_DATA];
    uint32_t size;
    uint8_t is_used;
};

/* File system storage (static, in-RAM) */
extern struct File file_system[MAX_FILES];

/* Basic file operations */
int fs_create(const char *name);
int fs_write(const char *name, const char *content);
struct File* fs_read(const char *name);
int fs_delete(const char *name);

/* Shell-like commands */
void cmd_ls(void);
void cmd_touch(const char *name);
void cmd_cat(const char *name);
void cmd_rm(const char *name);
void cmd_write(const char *name, const char *text);

/* Power commands */
void cmd_halt(void);
void cmd_reboot(void);

#endif /* FS_H */

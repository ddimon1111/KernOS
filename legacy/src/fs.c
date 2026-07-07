/* Duplicate of top-level fs.c for building from src/ */
#include "fs.h"
#include "vga.h"

#include <stdint.h>

/* In-memory file table */
struct File file_system[MAX_FILES];

/* Minimal string helpers (freestanding). */
static int str_eq(const char *a, const char *b) {
    if (!a || !b) return 0;
    while (*a && *b) {
        if (*a != *b) return 0;
        ++a; ++b;
    }
    return (*a == '\0' && *b == '\0');
}

static void str_cpy(char *dst, const char *src, int maxlen) {
    int i = 0;
    while (i < maxlen - 1 && src[i]) {
        dst[i] = src[i];
        i++;
    }
    dst[i] = '\0';
}

/* Find a file index by name, or -1 if not found */
static int find_file_index(const char *name) {
    for (int i = 0; i < MAX_FILES; ++i) {
        if (file_system[i].is_used && str_eq(file_system[i].name, name))
            return i;
    }
    return -1;
}

int fs_create(const char *name) {
    if (!name) return -1;
    /* Check for existing */
    if (find_file_index(name) != -1) return -1;

    for (int i = 0; i < MAX_FILES; ++i) {
        if (!file_system[i].is_used) {
            file_system[i].is_used = 1;
            str_cpy(file_system[i].name, name, MAX_NAME_LEN);
            file_system[i].size = 0;
            file_system[i].data[0] = '\0';
            return 0;
        }
    }
    return -1; /* no space */
}

int fs_write(const char *name, const char *content) {
    if (!name || !content) return -1;
    int idx = find_file_index(name);
    if (idx == -1) return -1;
    /* Copy up to MAX_FILE_DATA-1 bytes */
    int i = 0;
    while (i < MAX_FILE_DATA - 1 && content[i]) {
        file_system[idx].data[i] = content[i];
        ++i;
    }
    file_system[idx].data[i] = '\0';
    file_system[idx].size = (uint32_t)i;
    return 0;
}

struct File* fs_read(const char *name) {
    int idx = find_file_index(name);
    if (idx == -1) return 0;
    return &file_system[idx];
}

int fs_delete(const char *name) {
    int idx = find_file_index(name);
    if (idx == -1) return -1;
    file_system[idx].is_used = 0;
    file_system[idx].name[0] = '\0';
    file_system[idx].data[0] = '\0';
    file_system[idx].size = 0;
    return 0;
}

/* Command implementations */
void cmd_ls(void) {
    vga_print("Files:\n", 0x00FFFF00U); /* yellow tag */
    for (int i = 0; i < MAX_FILES; ++i) {
        if (file_system[i].is_used) {
            vga_print(" - ", 0x00FFFFFFU);
            vga_print(file_system[i].name, 0x00FFFFFFU);
            vga_print(" (", 0x00AAAAAAU);
            vga_print_num(file_system[i].size, 0x00FFFFFFU);
            vga_print(" bytes)\n", 0x00AAAAAAU);
        }
    }
}

void cmd_touch(const char *name) {
    if (fs_create(name) == 0) {
        vga_print("Created ", 0x0000FFFFU);
        vga_print(name, 0x00FFFFFFU);
        vga_print("\n", 0x00FFFFFFU);
    } else {
        vga_print("Failed to create ", 0x00FF0000U);
        vga_print(name, 0x00FFFFFFU);
        vga_print("\n", 0x00FFFFFFU);
    }
}

void cmd_cat(const char *name) {
    struct File *f = fs_read(name);
    if (!f) {
        vga_print("No such file: ", 0x00FF0000U);
        vga_print(name, 0x00FFFFFFU);
        vga_print("\n", 0x00FFFFFFU);
        return;
    }
    /* Assume text data */
    vga_print(f->data, 0x00FFFFFFU);
    vga_print("\n", 0x00FFFFFFU);
}

void cmd_rm(const char *name) {
    if (fs_delete(name) == 0) {
        vga_print("Removed ", 0x0000FFFFU);
        vga_print(name, 0x00FFFFFFU);
        vga_print("\n", 0x00FFFFFFU);
    } else {
        vga_print("Failed to remove ", 0x00FF0000U);
        vga_print(name, 0x00FFFFFFU);
        vga_print("\n", 0x00FFFFFFU);
    }
}

void cmd_write(const char *name, const char *text) {
    if (fs_write(name, text) == 0) {
        vga_print("Wrote to ", 0x0000FFFFU);
        vga_print(name, 0x00FFFFFFU);
        vga_print("\n", 0x00FFFFFFU);
    } else {
        vga_print("Failed to write to ", 0x00FF0000U);
        vga_print(name, 0x00FFFFFFU);
        vga_print("\n", 0x00FFFFFFU);
    }
}

/* Halt: stop CPU by repeatedly executing HLT */
void cmd_halt(void) {
    vga_print("System halted.\n", 0x00FFFFFFU);
    for (;;) { __asm__ volatile ("hlt"); }
}

/* Reboot: attempt keyboard controller reset (port 0x64, command 0xFE). */
static inline void outb(uint16_t port, uint8_t val) {
    __asm__ volatile ("outb %0, %1" : : "a" (val), "Nd" (port));
}

void cmd_reboot(void) {
    vga_print("Rebooting...\n", 0x00FFFFFFU);
    /* Try keyboard controller reset */
    outb(0x64, 0xFE);
    /* If that fails, halt */
    for (;;) { __asm__ volatile ("hlt"); }
}

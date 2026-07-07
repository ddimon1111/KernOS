#include "vga.h"
#include "idt.h"
#include "fs.h"

#include <stdint.h>

#define COLOR_BG   0x07U /* white on black */
#define COLOR_TAG  0x0FU /* bright white */
#define COLOR_VAL  0x07U /* normal white */

void do_mini_fastfetch(void) {
    vga_print("OS: KernOS BIOS\n", COLOR_VAL);
    vga_print("KERNEL: Freestanding C x86_64\n", COLOR_VAL);
    vga_print("BOOT: custom BIOS loader\n", COLOR_VAL);
}

void kmain(void) {
    vga_init();
    vga_clear(COLOR_BG);

    idt_init();
    serial_init();
    __asm__ volatile ("sti");

    serial_print("KernOS booted from custom BIOS loader\n");
    vga_print("KernOS booted from custom BIOS loader\n", COLOR_VAL);
    do_mini_fastfetch();

    for (;;) {
        __asm__ volatile ("hlt");
    }
}

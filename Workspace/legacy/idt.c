#include "idt.h"
#include "vga.h"

#include <stdint.h>

/* I/O port helpers */
static inline void outb(uint16_t port, uint8_t val) {
    __asm__ volatile ("outb %0, %1" : : "a" (val), "Nd" (port));
}

static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    __asm__ volatile ("inb %1, %0" : "=a" (ret) : "Nd" (port));
    return ret;
}

/* IDT structures */
struct idt_entry {
    uint16_t offset_low;
    uint16_t selector;
    uint8_t ist;
    uint8_t type_attr;
    uint16_t offset_mid;
    uint32_t offset_high;
    uint32_t zero;
} __attribute__((packed));

struct idt_ptr {
    uint16_t limit;
    uint64_t base;
} __attribute__((packed));

static struct idt_entry idt[256];

volatile char input_buffer[256];
volatile int input_pos = 0;
volatile int input_ready = 0;

/* Basic PS/2 set 1 scancode -> ASCII map (no shift handling) */
static const unsigned char scancode_map[256] = {
    [0x02] = '1', [0x03] = '2', [0x04] = '3', [0x05] = '4', [0x06] = '5',
    [0x07] = '6', [0x08] = '7', [0x09] = '8', [0x0A] = '9', [0x0B] = '0',
    [0x0C] = '-', [0x0D] = '=',
    [0x0E] = '\b', /* backspace */
    [0x0F] = '\t',
    [0x10] = 'q', [0x11] = 'w', [0x12] = 'e', [0x13] = 'r', [0x14] = 't',
    [0x15] = 'y', [0x16] = 'u', [0x17] = 'i', [0x18] = 'o', [0x19] = 'p',
    [0x1A] = '[', [0x1B] = ']',
    [0x1C] = '\n', /* Enter */
    [0x1E] = 'a', [0x1F] = 's', [0x20] = 'd', [0x21] = 'f', [0x22] = 'g',
    [0x23] = 'h', [0x24] = 'j', [0x25] = 'k', [0x26] = 'l',
    [0x2C] = 'z', [0x2D] = 'x', [0x2E] = 'c', [0x2F] = 'v', [0x30] = 'b',
    [0x31] = 'n', [0x32] = 'm',
    [0x39] = ' ',
};

/* Set one IDT entry by absolute handler address */
static void set_idt_entry(int n, uint64_t handler_addr) {
    uint64_t addr = handler_addr;
    idt[n].offset_low = (uint16_t)(addr & 0xFFFF);
    idt[n].selector = 0x08; /* code segment selector */
    idt[n].ist = 0;
    idt[n].type_attr = 0x8E; /* present, DPL=0, interrupt gate */
    idt[n].offset_mid = (uint16_t)((addr >> 16) & 0xFFFF);
    idt[n].offset_high = (uint32_t)((addr >> 32) & 0xFFFFFFFF);
    idt[n].zero = 0;
}

/* Low-level keyboard handler (C, called from interrupt). Using GCC's
 * interrupt attribute would create correct prologue/epilogue; instead we
 * implement a simple naked wrapper in assembly below that calls this C
 * function by placing scancode argument in AL (we read port here).
 */
void keyboard_handle_make(uint8_t scancode) {
    if (scancode & 0x80) {
        /* key release */
        return;
    }

    unsigned char c = scancode_map[scancode];
    if (!c) return;

    if (c == '\b') {
        if (input_pos > 0) {
            input_pos--;
            vga_backspace();
        }
    } else if (c == '\n') {
        /* Terminate buffer and mark ready */
        input_buffer[input_pos] = '\0';
        input_ready = 1;
        /* Echo newline */
        vga_putchar('\n', 0x00FFFFFFU);
    } else {
        if (input_pos < (int)sizeof(input_buffer) - 1) {
            input_buffer[input_pos++] = (char)c;
            vga_putchar((char)c, 0x00FFFFFFU);
        }
    }
}

/* Assembly wrapper for keyboard ISR: read scancode, call keyboard_handle_make,
 * send EOI to PIC, and return via iretq. We'll define it as global symbol
 * keyboard_isr so we can place its address in the IDT.
 */
__attribute__((interrupt)) void keyboard_isr(void *frame) {
    uint8_t scancode = inb(0x60);
    keyboard_handle_make(scancode);
    /* Send EOI to PIC */
    outb(0x20, 0x20);
}

/* Remap PIC controllers to avoid conflicts with CPU exceptions */
static void pic_remap(void) {
    outb(0x20, 0x11);
    outb(0xA0, 0x11);
    outb(0x21, 0x20);
    outb(0xA1, 0x28);
    outb(0x21, 0x04);
    outb(0xA1, 0x02);
    outb(0x21, 0x01);
    outb(0xA1, 0x01);
    outb(0x21, 0x0);
    outb(0xA1, 0x0);
}

/* IDT initialization */
void idt_init(void) {
    for (int i = 0; i < 256; ++i) {
        set_idt_entry(i, 0);
    }

    /* Remap PIC then install keyboard ISR at vector 0x21 (IRQ1) */
    pic_remap();
    set_idt_entry(0x21, (uint64_t)keyboard_isr);

    struct idt_ptr p;
    p.limit = sizeof(idt) - 1;
    p.base = (uint64_t)&idt;
    __asm__ volatile ("lidt %0" : : "m" (p));

    /* Unmask keyboard IRQ (PIC IMR) — enable IRQ1 only */
    outb(0x21, 0xFD); /* 11111101b -> mask all but IRQ1 */
    outb(0xA1, 0xFF);
}

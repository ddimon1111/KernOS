#ifndef IDT_H
#define IDT_H

#include <stdint.h>

/* Initialize IDT, PIC remapping, and install keyboard IRQ handler */
void idt_init(void);

/* Input buffer shared with keyboard ISR and kernel REPL */
extern volatile char input_buffer[256];
extern volatile int input_pos;
extern volatile int input_ready; /* set to 1 when Enter pressed */

#endif /* IDT_H */

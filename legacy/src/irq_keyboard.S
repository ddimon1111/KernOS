/* Assembly ISR stub for keyboard IRQ (IRQ1)
 * Reads scancode from port 0x60, calls keyboard_handle_make(uint8_t),
 * sends EOI to PIC (port 0x20), and iretq.
 * Uses AT&T syntax for GNU assembler.
 */
    .text
    .global keyboard_isr
    .type keyboard_isr, @function
keyboard_isr:
    /* Save general-purpose registers */
    pushq %rax
    pushq %rcx
    pushq %rdx
    pushq %rsi
    pushq %rdi
    pushq %r8
    pushq %r9
    pushq %r10
    pushq %r11
    pushq %r12
    pushq %r13
    pushq %r14
    pushq %r15

    /* Read scancode into AL */
    inb $0x60, %al
    /* Zero-extend AL into RDI (first integer arg) */
    movzbq %al, %rdi

    /* Call C handler: keyboard_handle_make(uint8_t) */
    call keyboard_handle_make

    /* Send EOI to PIC (port 0x20, value 0x20) */
    movb $0x20, %al
    outb %al, $0x20

    /* Restore registers in reverse order */
    popq %r15
    popq %r14
    popq %r13
    popq %r12
    popq %r11
    popq %r10
    popq %r9
    popq %r8
    popq %rdi
    popq %rsi
    popq %rdx
    popq %rcx
    popq %rax

    iretq

    .size keyboard_isr, .-keyboard_isr

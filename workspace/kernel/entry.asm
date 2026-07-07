; Kernel entry point for KernOS.
; This is the first code to run after the bootloader transfers control to the kernel.
; It sets up a minimal stack and calls the C entry function kmain().

BITS 64

global _start
extern kmain

section .text
_start:
    xor rbp, rbp
    lea rsp, [rel stack_top]
    call kmain

halt_loop:
    cli
    hlt
    jmp halt_loop

section .bss
align 16
stack_space: resb 8192
stack_top:

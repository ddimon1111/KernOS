; Global Descriptor Table for Stage2 protected-mode and long-mode entry.
; Stage2 embeds an equivalent table inline in the flat binary image.

BITS 16

section .data
align 8

gdt_null:
    dq 0x0000000000000000

; 32-bit flat code segment (selector 0x08)
gdt_code32:
    dq 0x00CF9A000000FFFF

; 32-bit flat data segment (selector 0x10)
gdt_data32:
    dq 0x00CF92000000FFFF

; 64-bit long-mode code segment (selector 0x18)
gdt_code64:
    dq 0x00AF9A000000FFFF

; 64-bit data segment (selector 0x20)
gdt_data64:
    dq 0x00CF92000000FFFF

gdt_end:

gdt_descriptor:
    dw gdt_end - gdt_null - 1
    dd gdt_null

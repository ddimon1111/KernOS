/* Linker script for the kernel binary.
   The kernel is linked with a 0x0 base, then loaded by Stage2 at physical address 0x100000.
   This keeps the binary compact while allowing later relocation by the bootloader. */

OUTPUT_FORMAT("binary")
ENTRY(_start)

SECTIONS
{
    . = 0x0;
    .text : { *(.text*) }
    .rodata : { *(.rodata*) }
    .data : { *(.data*) }
    .bss : { *(.bss*) }
}

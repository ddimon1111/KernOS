/* Linker script for stage2 when stage2 is built as a linked binary.
   The stage2 raw binary is loaded by Stage1 at physical address 0x9000. */

OUTPUT_FORMAT("binary")
ENTRY(start)

SECTIONS
{
    . = 0x9000;
    .text : { *(.text) }
    .rodata : { *(.rodata) }
    .data : { *(.data) }
    .bss : { *(.bss) }
}

; Identity-mapped page tables for Stage2 long-mode transition.
; Maps the first 2 MiB of physical memory to itself using one 2 MiB page.
;
; These tables are reference definitions. Stage2 embeds equivalent tables
; inline so the boot image stays a single flat binary.

BITS 32

section .bss
align 4096
pml4_table:
    resq 512

align 4096
pdpt_table:
    resq 512

align 4096
pd_table:
    resq 512

section .text
global setup_paging_tables

; Fill page tables with physical addresses ORed with required flags.
setup_paging_tables:
    ; PML4[0] -> PDPT
    mov eax, pdpt_table
    or eax, 0x03
    mov dword [pml4_table], eax
    mov dword [pml4_table + 4], 0

    ; PDPT[0] -> PD
    mov eax, pd_table
    or eax, 0x03
    mov dword [pdpt_table], eax
    mov dword [pdpt_table + 4], 0

    ; PD[0] -> 2 MiB page frame 0 (covers 0x00000000-0x001FFFFF)
    mov dword [pd_table], 0x00000083
    mov dword [pd_table + 4], 0
    ret

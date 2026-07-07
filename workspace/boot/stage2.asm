; Stage2 loader for KernOS.
; Loaded by Stage1 at physical address 0x00009000 (ORG 0x9000).
;
; Real mode:
; - Enable the A20 line
; - Load the kernel below 1 MB (staging buffer at 0x00080000)
;
; Protected mode (32-bit):
; - Copy the kernel to its final address at 0x00100000
; - Install identity page tables and enable PAE, LME, and paging
;
; Long mode (64-bit):
; - Jump to the kernel linked for load address 0x00100000

BITS 16
ORG 0x9000

BOOT_DRIVE_ADDR     equ 0x7E00
KERNEL_STAGING_PHYS equ 0x00080000
KERNEL_DEST_PHYS    equ 0x00100000
KERNEL_START_SECTOR equ 33
KERNEL_SECTORS      equ 64
KERNEL_COPY_DWORDS  equ (KERNEL_SECTORS * 512) / 4

; ---------------------------------------------------------------------------
; Real-mode entry and helpers (must stay in 16-bit mode)
; ---------------------------------------------------------------------------

start:
    cli
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x9F00

    mov si, msg_stage2
    call print_string

    call enable_a20
    call load_kernel

    mov si, msg_jump
    call print_string

    call setup_paging

    mov eax, cr4
    or eax, 1 << 5
    mov cr4, eax

    lgdt [gdt_descriptor]

    mov eax, cr0
    or eax, 1
    mov cr0, eax

    jmp 0x08:protected_mode_entry

enable_a20:
    in al, 0x92
    or al, 2
    out 0x92, al
    ret

load_kernel:
    mov ax, KERNEL_STAGING_PHYS >> 4
    mov es, ax
    xor bx, bx

    mov ah, 0x02
    mov al, KERNEL_SECTORS
    mov ch, 0
    mov cl, KERNEL_START_SECTOR
    mov dh, 0
    mov dl, [BOOT_DRIVE_ADDR]
    int 0x13
    jc .disk_error
    ret

.disk_error:
    mov si, msg_fail
    call print_string
    jmp halt

setup_paging:
    mov eax, pdpt_table
    or eax, 0x03
    mov dword [pml4_table], eax
    mov dword [pml4_table + 4], 0

    mov eax, pd_table
    or eax, 0x03
    mov dword [pdpt_table], eax
    mov dword [pdpt_table + 4], 0

    mov dword [pd_table], 0x00000083
    mov dword [pd_table + 4], 0
    ret

print_string:
    mov ah, 0x0E
.next_char:
    lodsb
    cmp al, 0
    je .done
    int 0x10
    jmp .next_char
.done:
    ret

halt:
    cli
.halt_loop:
    hlt
    jmp .halt_loop

msg_stage2: db 'Stage2 loaded at 0x9000', 0x0D, 0x0A, 0
msg_jump:   db 'Jumping to kernel at 0x100000...', 0x0D, 0x0A, 0
msg_fail:   db 'Kernel load failed. Halting.', 0x0D, 0x0A, 0

align 8
gdt_null:
    dq 0
gdt_code32:
    dq 0x00CF9A000000FFFF
gdt_data32:
    dq 0x00CF92000000FFFF
gdt_code64:
    dq 0x00AF9A000000FFFF
gdt_data64:
    dq 0x00CF92000000FFFF

gdt_descriptor:
    dw gdt_data64 - gdt_null - 1
    dd gdt_null

; ---------------------------------------------------------------------------
; 32-bit protected mode
; ---------------------------------------------------------------------------

align 16
BITS 32
protected_mode_entry:
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax

    mov esi, KERNEL_STAGING_PHYS
    mov edi, KERNEL_DEST_PHYS
    mov ecx, KERNEL_COPY_DWORDS
    rep movsd

    mov eax, pml4_table
    mov cr3, eax

    mov ecx, 0xC0000080
    rdmsr
    or eax, 1 << 8
    wrmsr

    mov eax, cr0
    or eax, 1 << 31
    mov cr0, eax

    jmp 0x18:long_mode_entry

; ---------------------------------------------------------------------------
; 64-bit long mode
; ---------------------------------------------------------------------------

align 16
BITS 64
long_mode_entry:
    mov ax, 0x20
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax

    mov rsp, 0x00090000
    xor rbp, rbp

    mov rax, KERNEL_DEST_PHYS
    jmp rax

; ---------------------------------------------------------------------------
; Page tables (physical addresses used by CR3 and setup_paging)
; ---------------------------------------------------------------------------

align 4096
pml4_table:
    times 512 dq 0

align 4096
pdpt_table:
    times 512 dq 0

align 4096
pd_table:
    times 512 dq 0

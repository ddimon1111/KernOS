; Stage1 boot sector for KernOS
; Loaded by the BIOS at physical address 0x00007C00 (segment 0x0000, offset 0x7C00).
;
; Responsibilities:
; - Print a status message
; - Preserve the boot drive number passed in DL by the BIOS
; - Load Stage2 from disk into physical address 0x00009000
; - Jump to Stage2

BITS 16
ORG 0x7C00

BOOT_DRIVE_ADDR     equ 0x7E00
STAGE2_LOAD_OFFSET  equ 0x9000
STAGE2_FIRST_SECTOR equ 2       ; CHS sector 2 = LBA 1 (sector 1 is this boot sector)
STAGE2_SECTORS      equ 32
STAGE2_RETRIES      equ 3

start:
    cli
    xor ax, ax
    mov ss, ax
    mov sp, 0x7C00
    mov ds, ax
    mov es, ax

    mov si, msg_stage1
    call print_string

    mov [BOOT_DRIVE_ADDR], dl

    mov bl, STAGE2_RETRIES
load_stage2:
    mov ah, 0x02
    mov al, STAGE2_SECTORS
    mov ch, 0
    mov cl, STAGE2_FIRST_SECTOR
    mov dh, 0
    mov dl, [BOOT_DRIVE_ADDR]
    mov bx, STAGE2_LOAD_OFFSET
    xor ax, ax
    mov es, ax
    int 0x13
    jc disk_error
    jmp stage2_loaded

disk_error:
    dec bl
    jz load_failed
    call print_retry
    jmp load_stage2

load_failed:
    mov si, msg_fail
    call print_string
    cli
    hlt
    jmp $

stage2_loaded:
    ; Stage2 is linked for ORG 0x9000; jump to its entry point.
    jmp 0x0000:STAGE2_LOAD_OFFSET

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

print_retry:
    mov si, msg_retry
    call print_string
    ret

msg_stage1: db 'Stage1 loaded. Loading Stage2...', 0x0D, 0x0A, 0
msg_retry:  db 'Stage2 load failed, retrying...', 0x0D, 0x0A, 0
msg_fail:   db 'Stage2 load failed after retries. Halting.', 0x0D, 0x0A, 0

times 510-($-$$) db 0
dw 0xAA55

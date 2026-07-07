; BIOS disk helpers for KernOS boot stages.
; These routines are written in 16-bit real mode and may be used by bootloader stages.

BITS 16

section .text

global disk_read_sectors
; Read sectors from the boot disk using CHS addressing.
; Input:
;   AH = 0x02 (read sectors)
;   AL = number of sectors
;   CH = cylinder (0..1023 low 8 bits)
;   CL = sector (1..63) + ((cylinder >> 8) << 6)
;   DH = head
;   DL = drive number
;   ES:BX = buffer address
; Output:
;   CF = 0 on success, 1 on error
;   AH = status code on error
disk_read_sectors:
    push ax
    push bx
    push cx
    push dx
    int 0x13
    pop dx
    pop cx
    pop bx
    pop ax
    ret

global disk_int15_e820
; Query the BIOS E820 memory map.
; Input:
;   EBX = continuation value (0 for first call)
;   ES:DI = address of a 24-byte buffer for one entry.
; Output:
;   CF = 0 on success, 1 on error
;   EBX = continuation value for next call; 0 when finished
;   EAX = 'SMAP'
;   EDX = 'SMAP'
;   ECX = actual buffer size returned
disk_int15_e820:
    push eax
    push ebx
    push ecx
    push edx
    push si
    push di
    push ds
    push es

    mov ax, 0x0000
    mov ds, ax
    mov ax, 0x0000
    mov es, ax

    mov ax, 0xE820
    mov dx, 'SM'
    mov word [e820_sig+0], dx
    mov dx, 'AP'
    mov word [e820_sig+2], dx

    mov eax, 0xE820
    mov edx, 0x534D4150      ; 'SMAP' in little-endian
    mov ecx, 24              ; Size of buffer in bytes.
    call set_es_di
    int 0x15

    pop es
    pop ds
    pop di
    pop si
    pop edx
    pop ecx
    pop ebx
    pop eax
    ret

set_es_di:
    ; Convert ES:DI from a 32-bit pointer in DS:SI to a real-mode pointer.
    ; In this helper, DS:SI holds the pointer.
    mov bx, si
    mov es, ds
    mov di, bx
    ret

section .data
    e820_sig: times 4 db 0

bits 16

section .entry

extern __bss_start
extern __end
extern _init

extern cstart
global entry

entry:
    cli

    ; info from stage 1
    mov [g_boot_drive], dl
    mov [g_partition_info.lba], ebx
    mov [g_partition_info.size], cl

    ; set up segments
    mov ax, ds
    mov ss, ax
    mov sp, 0xBFFF
    mov bp, sp

    ; switch to protected mode
    call enable_a20     ;; enable the a20 gate
    call load_gdt       ;; load global descriptor table

    ; set protected mode enabled flag in cr0
    mov eax, cr0
    or al, 1
    mov cr0, eax

    ; far jump into 32 bit protected mode
    jmp dword 0x8:.32_bit_protected_mode

[bits 32]

.32_bit_protected_mode:
    ; this is now in 32 bit protected mode

    ; set up segments
    mov ax, 0x10
    mov ds, ax
    mov ss, ax

    ; empty bss
    mov edi, __bss_start
    mov ecx, __end
    sub ecx, edi
    mov al, 0
    cld
    rep stosb

    ; call global contructors
    call _init

    ; --- partition table entry address parameter
    ; offset
    mov edx, [g_partition_info.size]
    push edx
    ; segment
    mov edx, [g_partition_info.lba]
    push edx

    ; boot drive parameter
    xor edx, edx
    mov dl, [g_boot_drive]
    push edx

    ; call kernel
    call cstart

    cli
    hlt

[bits 16]

enable_a20:
    ; disable keyboard
    call a20_wait_input
    mov al, KeyboardConrollerDisableKeyboard
    out KeyboardConrollerCommandPort, al

    ; read control output port
    call a20_wait_input
    mov al, KeyboardConrollerReadControlOutputPort
    out KeyboardConrollerCommandPort, al

    call a20_wait_output
    in al, KeyboardConrollerDataPort
    push eax

    ; write control output port
    call a20_wait_input
    mov al, KeyboardConrollerWriteControlOutputPort
    out KeyboardConrollerCommandPort, al

    call a20_wait_input
    pop eax
    or al, 2
    out KeyboardConrollerDataPort, al

    ; re-enable keyboard
    call a20_wait_input
    mov al, KeyboardConrollerEnableKeyboard
    out KeyboardConrollerCommandPort, al

    call a20_wait_input
    ret

a20_wait_input:
    in al, KeyboardConrollerCommandPort
    test al, 2
    jnz a20_wait_input
    ret

a20_wait_output:
    in al, KeyboardConrollerCommandPort
    test al, 1
    jz a20_wait_output
    ret

load_gdt:
    lgdt [g_gdt_descriptor]
    ret

KeyboardConrollerDataPort               equ 0x60
KeyboardConrollerCommandPort            equ 0x64
KeyboardConrollerDisableKeyboard        equ 0xAD
KeyboardConrollerEnableKeyboard         equ 0xAE
KeyboardConrollerReadControlOutputPort  equ 0xD0
KeyboardConrollerWriteControlOutputPort equ 0xD1

g_gdt:
    dq 0

    ; 32-bit code segment
    dw 0xFFFF
    dw 0
    db 0
    db 10011010b        ;; access (present, ring 0, code segment, executable, direction 0, readable)
    db 11001111b        ;; granularity (4kb pages, 32-bit protected mode) +  limit (bits 16-19)
    db 0

    ; 32-bit data segment
    dw 0xFFFF
    dw 0
    db 0
    db 10010010b        ;; access (present, ring 0, data segment, executable, direction 0, writable)
    db 11001111b        ;; granularity (4kb pages, 32-bit protected mode) +  limit (bits 16-19)
    db 0

    ; 16-bit code segment
    dw 0xFFFF
    dw 0
    db 0
    db 10011010b        ;; access (present, ring 0, code segment, executable, direction 0, readable)
    db 00001111b        ;; granularity (1 byte pages, 16-bit protected mode) +  limit (bits 16-19)
    db 0

    ; 16-bit data segment
    dw 0xFFFF
    dw 0
    db 0
    db 10010010b        ;; access (present, ring 0, data segment, executable, direction 0, writable)
    db 00001111b        ;; granularity (1 byte pages, 16-bit protected mode) +  limit (bits 16-19)
    db 0

g_gdt_descriptor:
    dw g_gdt_descriptor - g_gdt - 1     ;; limit = size of gdt
    dd g_gdt

g_boot_drive:
    db 0

g_partition_info:
    .lba        dd 0
    .size       db 0

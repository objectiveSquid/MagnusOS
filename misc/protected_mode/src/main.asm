org 0x7C00
bits 16

entry:
    ; set up segments
    mov ax, 0
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7C00

    ; switch to protected mode
    cli                 ;; disable interrupts
    call enable_a20     ;; enable the a20 gate
    call load_gdt       ;; load global descriptor table

    ; set protected mode enabled flag in cr0
    mov eax, cr0
    or al, 1
    mov cr0, eax

    ; far jump into 32 bit protected mode
    jmp dword 0x8:.32_bit_protected_mode

.32_bit_protected_mode:
    [bits 32]
    ; this is now in 32 bit protected mode

    ; set up segments
    mov ax, 0x10
    mov ds, ax
    mov ss, ax

    ; print hello message
    mov esi, g_msg_hello_32_bit_protected_mode
    mov edi, ScreenBuffer
    cld

    mov bl, 0

.32_bit_protected_mode_print_loop:
    lodsb
    cmp al, 0x00
    je .32_bit_protected_mode_print_done

    mov [edi], al
    inc edi

    add bl, 1
    mov [edi], bl
    inc edi
    jmp .32_bit_protected_mode_print_loop

.32_bit_protected_mode_print_done:
    ; far jump into 16 bit protected mode
    jmp word 0x18:.16_bit_protected_mode

[bits 16]

.16_bit_protected_mode:
    ; unset protected mode enabled flag in cr0
    mov eax, cr0
    and eax, ~1
    mov cr0, eax

    ; jump to real mode
    jmp word 0x00:.real_mode

.real_mode:
    ; set up segments
    mov ax, 0
    mov ds, ax
    mov ss, ax

    ; re-enable interrupts
    sti

    ; print hello message
    mov si, g_msg_hello_16_bit_real_mode

.16_bit_real_mode_print_loop:
    lodsb
    cmp al, 0x00
    je .16_bit_real_mode_print_done

    mov ah, 0xE
    int 0x10
    jmp .16_bit_real_mode_print_loop

.16_bit_real_mode_print_done:

endless_loop:
    jmp endless_loop

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

ScreenBuffer                            equ 0xB8000

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

g_msg_hello_32_bit_protected_mode:
    db "Hello from 32-bit protected mode!", 0x00
g_msg_hello_16_bit_real_mode:
    db "Hello from 16-bit real mode!", 0x00

times 510-($-$$) db 0
db 0x55, 0xAA
[bits 32]

extern i686_ISR_Handler

%macro ISR_NOERRORCODE 1

global i686_ISR_%1
i686_ISR_%1:
    push 0 ;; dummy error code
    push %1 ;; interrupt number
    jmp isr_common
    
%endmacro

%macro ISR_ERRORCODE 1
global i686_ISR_%1
i686_ISR_%1:
    ; interrupt already pushes an error code for us
    push %1 ;; interrupt number
    jmp isr_common
%endmacro

%include "lib/interrupt/isr/isr_gen.inc"

isr_common:
    pusha ;; save regs

    xor eax, eax
    mov ax, ds
    push eax

    mov ax, 0x10 ;; use kernel segments
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    push esp
    call i686_ISR_Handler
    add esp, 4

    pop eax ;; restore old segments
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    popa ;; restore regs
    add esp, 8
    iret
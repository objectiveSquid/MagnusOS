bits 32

global x86_OutByte
x86_OutByte:
    mov dx, [esp + 4]
    mov al, [esp + 8]
    out dx, al
    ret

global x86_InByte
x86_InByte:
    mov dx, [esp + 4]
    xor eax, eax
    in al, dx
    ret

global x86_OutWord
x86_OutWord:
    mov dx, [esp + 4]
    mov ax, [esp + 8]
    out dx, ax
    ret

global x86_InWord
x86_InWord:
    mov dx, [esp + 4]
    xor eax, eax
    in ax, dx
    ret

global x86_Halt
x86_Halt:
    cli
    hlt

global x86_EnableInterrupts
x86_EnableInterrupts:
    sti
    ret

global x86_DisableInterrupts
x86_DisableInterrupts:
    cli
    ret

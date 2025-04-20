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

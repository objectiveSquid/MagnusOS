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

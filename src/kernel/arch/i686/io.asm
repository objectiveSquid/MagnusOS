[bits 32]

global i686_OutByte
i686_OutByte:
    mov dx, [esp + 4]
    mov al, [esp + 8]
    out dx, al
    ret

global i686_InByte
i686_InByte:
    mov dx, [esp + 4]
    xor eax, eax
    in al, dx
    ret

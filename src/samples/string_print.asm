[org 0x7c00]

mov ah, 0x0e
mov bx, my_string

print_string:
    mov al, [bx]
    cmp al, 0x00
    je end
    int 0x10
    inc bx
    jmp print_string
end:
    jmp $

my_string:
    db "Hello world!", 0x00

times 510-($-$$) db 0
db 0x55, 0xaa
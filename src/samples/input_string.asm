mov bx, buffer

input_string:
    mov ah, 0x00
    int 0x16
    mov [bx], al
    inc bx
    cmp bx, buffer + 5
    je print_string
    jmp input_string

print_string:
    mov bx, buffer
    mov ah, 0x0e
print_string_loop:
    mov al, [bx]
    cmp al, 0x00
    je end
    int 0x10
    inc bx
    jmp print_string_loop
end:
    jmp $

buffer:
    times 10 db 0

times 510-($-$$) db 0
db 0x55, 0xaa
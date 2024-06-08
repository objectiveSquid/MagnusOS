mov ah, 0x0e
mov al, 'A'
int 0x10

loop:
    inc al
    cmp al, 'Z' + 1
    je end
    cmp al, 'z' + 1
    je end

    test al, 1
    jnz print_char
    add al, 32
print_char:
    int 0x10
    test al, 1
    jnz loop
    sub al, 32
    jmp loop
end:
    jmp $
times 510-($-$$) db 0
db 0x55, 0xaa
main:
    call my_func
    jmp main

my_func:
    mov bh, 'A'
my_func_loop:
    mov ah, 0x0e
    mov al, bh
    cmp al, 'Z' + 1
    je end_of_func
    int 0x10
    inc bh
    jmp my_func_loop
end_of_func:
    ret

times 510-($-$$) db 0
db 0x55, 0xaa
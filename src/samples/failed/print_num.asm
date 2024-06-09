;; THIS DOESNT WORK

mov al, [my_number]
mov bx, end_of_output_string - 1
mov cl, 0x00

calculate_number_loop:
    cmp cl, 0x01
    jne after_carry_eval
    inc byte [bx]
    mov cl, 0x00
after_carry_eval:
    dec al
    inc byte [bx]
    cmp byte [bx], '9' + 1
    jne after_carry
    mov cl, 0x01
    mov byte [bx], '0'
    dec bx
after_carry:
    cmp al, 0x00
    jne calculate_number_loop

    call print_string
    jmp $

print_string:
    mov bx, output_string
    mov ah, 0x0e
print_string_loop:
    cmp bx, end_of_output_string
    je end
    mov al, [bx]
    int 0x10
    inc bx
    jmp print_string_loop
end:
    ret

output_string:
    times 64 db '0'
end_of_output_string:

my_number:
    db 5

times 510-($-$$) db 0
db 0x55, 0xaa
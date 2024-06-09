org 0x7C00
bits 16

; Same as \r\n
%define ENDL 0x0D, 0x0A

start:
    jmp main

;
; Prints a string to the screen
; - Parameters:
;   - ds:si ---> points to a null-terminated string
;
puts:
    push si
    push ax
    mov ah, 0x0E
.puts_loop
    lodsb
    cmp al, 0x00
    je .puts_end
    int 0x10
    jmp .puts_loop
.puts_end
    pop ax
    pop si
    ret

main:
    ; Set up the data segments
    mov ax, 0
    mov ds, ax
    mov es, ax

    ; Set up the stack
    mov ss, ax
    mov sp, 0x7C00  ;; Stack pointer

    ; Print hello world
    mov si, msg_hello
    call puts

    hlt

.halt:
    jmp .halt

msg_hello:
    db "Hello world!", 0x00

times 510-($-$$) db 0
db 0x55, 0xAA
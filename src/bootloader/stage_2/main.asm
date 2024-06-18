org 0x0
bits 16

; Same as \r\n
%define ENDLINE 0x0D, 0x0A

start:
    ; Print hello message
    mov si, msg_hello
    call puts

    jmp halt

halt:
    cli
    hlt

; 
; Prints a string to the screen
; - Input parameters:
;   - ds:si ---> Points to a null-terminated string
; - Output:
;
puts:
    push si
    push ax
    mov ah, 0x0E
.puts_loop:
    lodsb
    cmp al, 0x00
    je .puts_end
    int 0x10
    jmp .puts_loop
.puts_end:
    pop ax
    pop si
    ret

msg_hello:
    db "Kernel started.", ENDLINE, 0x00

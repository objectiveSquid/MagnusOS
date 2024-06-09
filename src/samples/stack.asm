mov bp, 0x8000
mov sp, bp

mov bh, 'A'
push bx

mov ah, 0x0e
mov al, 'B'
int 0x10

pop bx
mov al, bh
int 0x10

times 510-($-$$) db 0
db 0x55, 0xaa
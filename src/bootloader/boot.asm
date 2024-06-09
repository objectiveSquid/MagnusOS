org 0x7C00
bits 16

; Same as \r\n
%define ENDL 0x0D, 0x0A

; FAT12 header
jmp short start
nop
bdb_oem:                    db 'MSWIN4.1'  ;; OEM identifier, must be 8 bytes
bdb_bytes_per_sector:       dw 512
bdb_bytes_per_cluster:      db 1
bdb_reserved_sectors:       dw 1
bdb_fat_count:              db 2
bdb_dir_entries_count:      dw 0E0h
bdb_total_sectors:          dw 2880  ;; 2880 * 512 = 1.44MB
bdb_media_descriptor_type:  db 0F0h  ;; F0 = 3.5" floppy disk
bdb_sectors_per_fat:        dw 9
bdb_sectors_per_track:      dw 18
bdb_heads:                  dw 2
bdb_hidden_sectors:         dd 0
bdb_large_sector_count:     dd 0

; Extended boot record
ebr_drive_number:           db 0  ;; 0x00 = Floppy, 0x80 = HDD
                            db 0  ;; Reserved
ebr_signature:              db 29h
ebr_volume_id:              db 12h, 34h, 56h, 78h  ;; Serial number, doesn't matter
ebr_volume_label:           db 'MagnusOS   '  ;; Label, must be 11 bytes
ebr_system_id:              db 'FAT12   '  ;; Filesystem id, must be 8 bytes

start:
    jmp main

; 
; Prints a string to the screen
; - Parameters:
;   - ds:si ---> Points to a null-terminated string
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
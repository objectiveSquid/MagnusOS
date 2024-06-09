org 0x7C00
bits 16

; Same as \r\n
%define ENDLINE 0x0D, 0x0A

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
; - Input parameters:
;   - ds:si ---> Points to a null-terminated string
; - Output:
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

    ; Read something from the disk
    mov [ebr_drive_number], dl

    mov ax, 1  ;; Second sector
    mov cl, 1
    mov bx, 0x7E00
    call read_lba

    ; Print hello messages
    mov si, msg_hello
    call puts

    jmp halt

floppy_error:
    mov si, msg_floppy_failed
    call puts

    jmp wait_key_and_reboot

wait_key_and_reboot:
    mov ah, 0x00
    int 0x16
    jmp 0FFFFh:0  ;; Jump to beginning of BIOS, thus rebooting 

halt:
    cli  ;; Disable interrupts, so the CPU cant exit the halt
    hlt

;
; Converts an LBA address into a CHS address
; - Input parameters:
;   - ax: An LBA address
; - Output:
;   - cx [bits 0-5]: Sector number
;   - cx [bits 6-15]: Cylinder number
;   - dh: Head number
;
lba_to_chs:
    push ax
    push dx

    xor dx, dx                          ;; dx = 0x00
    div word [bdb_sectors_per_track]    ;; ax = LBA / SectorsPerTrack
                                        ;; dx = LBA % SectorsPerTrack

    inc dx                              ;; dx = ((LBA % SectorsPerTrack) + 1) = Sector number
    mov cx, dx

    xor dx, dx                          ;; dx = 0x00
    div word [bdb_heads]                ;; ax = (LBA / SectorsPerTrack) / Heads
                                        ;; dx = (LBA / SectorsPerTrack) % Heads

    mov dh, dl
    mov ch, al
    shl ah, 6
    or cl, ah

    ; Restore registers
    pop ax
    mov dl, al
    pop ax
    ret

;
; Reads sectors from a disk
; - Input parameters:
;   - ax: An LBA adress
;   - cl: Number of sectors to read (up to 128)
;   - dl: Drive number
;   - es:bx: Output data
;
read_lba:
    push ax
    push bx
    push cx
    push dx
    push di

    push cx
    call lba_to_chs
    pop ax

    mov ah, 0x02
    mov di, 3           ;; Read retry count

.read_lba_retry:
    pusha
    stc
    int 0x13
    jnc .read_lba_done

    ; Failed to read from floppy
    popa
    call disk_reset

    dec di
    test di, di
    jnz .read_lba_retry
.read_lba_fail:
    jmp floppy_error
.read_lba_done:
    popa

    pop di
    pop dx
    pop cx
    pop bx
    pop ax

    ret

;
; Resets the disk controller
; - Inputs:
;   dl: Drive number to reset
disk_reset:
    pusha
    mov ah, 0x00
    stc
    int 0x13
    jc floppy_error
    popa
    ret

msg_hello:
    db "Hello world!", ENDLINE, 0x00

msg_floppy_failed:
    db "Failed to read floppy", ENDLINE, 0x00

times 510-($-$$) db 0
db 0x55, 0xAA
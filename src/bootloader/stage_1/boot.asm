org 0x7C00
bits 16

; Same as \r\n
%define ENDLINE 0x0D, 0x0A

; FAT12 header
jmp short start
nop
bdb_oem:                    db "MSWIN4.1"  ;; OEM identifier, must be 8 bytes
bdb_bytes_per_sector:       dw 512
bdb_bytes_per_cluster:      db 1
bdb_reserved_sectors:       dw 1
bdb_fat_count:              db 2
bdb_dir_entries_count:      dw 0xE0
bdb_total_sectors:          dw 2880  ;; 2880 * 512 = 1.44MB
bdb_media_descriptor_type:  db 0xF0  ;; F0 = 3.5" floppy disk
bdb_sectors_per_fat:        dw 9
bdb_sectors_per_track:      dw 18
bdb_heads:                  dw 2
bdb_hidden_sectors:         dd 0
bdb_large_sector_count:     dd 0

; Extended boot record
ebr_drive_number:           db 0  ;; 0x00 = Floppy, 0x80 = HDD
                            db 0  ;; Reserved
ebr_signature:              db 0x29
ebr_volume_id:              db 0x12, 0x34, 0x56, 0x78  ;; Serial number, doesn't matter
ebr_volume_label:           db "MagnusOS   "  ;; Label, must be 11 bytes
ebr_system_id:              db "FAT12   "  ;; Filesystem id, must be 8 bytes

times 90-($-$$) db 0

start:
    ; set up the data segments
    mov ax, 0
    mov ds, ax
    mov es, ax

    ; set up the stack
    mov ss, ax
    mov sp, 0x7C00  ;; stack pointer

    push es
    push word .after_basic_setup
    retf

.after_basic_setup:
    mov [ebr_drive_number], dl

    ; print hello message
    mov si, msg_hello
    call puts

    ; check extensions present
    mov ah, 0x41
    mov bx, 0x55AA
    stc
    int 0x13

    jc .no_disk_extensions
    cmp bx, 0xAA55  ;; technically not required
    jne .no_disk_extensions

    ; we know extensions are present
    mov byte [extensions_present], 1
    jmp .after_disk_extensions_check

.no_disk_extensions:
    mov byte [extensions_present], 0

.after_disk_extensions_check:
    ; load stage 2
    mov si, stage_2_location

    ; es:bx = stage 2 load address
    mov ax, STAGE_2_LOAD_SEGMENT
    mov es, ax
    mov bx, STAGE_2_LOAD_OFFSET

.load_stage_2_loop:
    mov eax, [si]  ;; lba
    add si, 4
    mov cl, [si]   ; length
    inc si

    cmp eax, 0
    je .read_stage_2_finish

    call disk_read

    ; move to next stage 2 load address
    xor ch, ch
    shl cx, 5
    mov di, es  ;; cant add directly to es
    add di, cx
    mov es, di

    jmp .load_stage_2_loop

.read_stage_2_finish:
    ; far jump to the second stage
    mov dl, [ebr_drive_number]              ;; dl = boot device

    mov ax, STAGE_2_LOAD_SEGMENT
    mov ds, ax
    mov es, ax

    jmp STAGE_2_LOAD_SEGMENT:STAGE_2_LOAD_OFFSET

    jmp wait_key_and_reboot                 ;; this should never run

    jmp halt                                ;; safeguard, also should never run

floppy_error:
    mov si, msg_floppy_failed
    call puts
    jmp wait_key_and_reboot

stage_2_not_found_error:
    mov si, msg_stage_2_not_found
    call puts
    jmp wait_key_and_reboot

wait_key_and_reboot:
    mov ah, 0x00
    int 0x16
    jmp 0xFFFF:0  ;; Jump to beginning of BIOS, thus rebooting 

halt:
    cli  ;; Disable interrupts, so the CPU cant exit the halt
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
;   - eax: An LBA adress
;   - cl: Number of sectors to read (up to 128)
;   - dl: Drive number
;   - es:bx: Output data
;
disk_read:
    push eax
    push bx
    push cx
    push dx
    push si
    push di

    cmp byte [extensions_present], 1
    jne .disk_read_no_extensions

    ; here is with extensions
    ; setup dap struct
    mov [extensions_dap.lba], eax
    mov [extensions_dap.segment], es
    mov [extensions_dap.offset], bx
    mov [extensions_dap.sector_count], cl

    mov ah, 0x42
    mov si, extensions_dap
    mov di, 3
    jmp .disk_read_retry

.disk_read_no_extensions:
    push cx
    call lba_to_chs
    pop ax

    mov ah, 0x02
    mov di, 3           ;; Read retry count

.disk_read_retry:
    pusha
    stc
    int 0x13
    jnc .disk_read_done

    ; Failed to read from floppy
    popa
    call disk_reset

    dec di
    test di, di
    jnz .disk_read_retry
.disk_read_fail:
    jmp floppy_error
.disk_read_done:
    popa

    pop di
    pop si
    pop dx
    pop cx
    pop bx
    pop eax

    ret

;
; Resets the disk controller
; - Inputs:
;   dl: Drive number to reset
;
disk_reset:
    pusha
    mov ah, 0x00
    stc
    int 0x13
    jc floppy_error
    popa
    ret

; texts
msg_hello:
    db "Loading MagnusOS...", ENDLINE, 0x00
msg_floppy_failed:
    db "Floppy disk error!", ENDLINE, 0x00
msg_stage_2_not_found:
    db "STAGE2.BIN file not found!", ENDLINE, 0x00
stage_2_filename:
    db "STAGE2  BIN"

; global vars
extensions_present:
    db 0

extensions_dap:
    .size           db 0x10
                    db 0
    .sector_count   dw 0
    .offset         dw 0
    .segment        dw 0
    .lba            dq 0

STAGE_2_LOAD_SEGMENT        equ 0x0
STAGE_2_LOAD_OFFSET         equ 0x500

times 510-30-($-$$) db 0

stage_2_location:
    times 30 db 0

db 0x55, 0xAA

misc_buffer:

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

start:
    ; Set up the data segments
    mov ax, 0
    mov ds, ax
    mov es, ax

    ; Set up the stack
    mov ss, ax
    mov sp, 0x7C00  ;; Stack pointer

    push es
    push word .after_basic_setup
    retf

.after_basic_setup:

    ; Read something from the disk
    mov [ebr_drive_number], dl

    ; Print hello messages
    mov si, msg_hello
    call puts

    ; read drive parameters
    push es
    mov ah, 0x8
    int 0x13
    jc floppy_error
    pop es

    and cl, 0x3F
    xor ch, ch
    mov [bdb_sectors_per_track], cx

    inc dh
    mov [bdb_heads], dh

    ; calculate lba of fat root directory
    mov ax, [bdb_sectors_per_fat]
    mov bl, [bdb_fat_count]
    xor bh, bh
    mul bx
    add ax, [bdb_reserved_sectors]          ;; ax = lba of fat root directory
    push ax                                 ;; save ax to the stack

    ; calculate size of fat root directory
    mov ax, [bdb_dir_entries_count]
    shl ax, 5                               ;; (ax << 5) == (ax *= 32)
    xor dx, dx
    div word [bdb_bytes_per_sector]

    test dx, dx
    jz .read_root_dir_after
    inc ax
.read_root_dir_after:
    ; read root directory
    mov cl, al
    pop ax
    mov dl, [ebr_drive_number]
    mov bx, misc_buffer
    call disk_read

    ; search for the stage_2 (bootloader_stage_2.bin)
    xor bx, bx
    mov di, misc_buffer         ;; the file name is the first 11 bytes of a directory entry
.search_stage_2_loop:
    mov si, stage_2_filename
    mov cx, 11
    push di
    repe cmpsb                      ;; repe = repeat until equal
                                    ;; cmpsb = compare (string) bytes, and decrement cx
    pop di
    je .found_stage_2

    add di, 32
    inc bx
    cmp bx, [bdb_dir_entries_count]
    jl .search_stage_2_loop          ;; jl = jump if less than
    jmp stage_2_not_found_error

.found_stage_2:
    mov ax, [di + 26]               ;; the first cluster position is at the 26th byte
    mov [stage_2_cluster], ax

    ; load fat from disk into memory
    mov ax, [bdb_reserved_sectors]
    mov cl, [bdb_sectors_per_fat]
    mov dl, [ebr_drive_number]
    mov bx, misc_buffer
    call disk_read

    ; read stage_2 and process fat chain
    mov bx, KERNEL_LOAD_SEGMENT
    mov es, bx
    mov bx, KERNEL_LOAD_OFFSET
.read_stage_2_loop:
    mov ax, [stage_2_cluster]

    add ax, 31
    mov cl, 1
    mov dl, [ebr_drive_number]
    call disk_read

    add bx, [bdb_bytes_per_sector]

    ; compute location of the next cluster
    mov ax, [stage_2_cluster]
    mov cx, 3
    mul cx
    mov cx, 2
    div cx

    mov si, misc_buffer
    add si, ax
    mov ax, [ds:si]

    or dx, dx
    jz .even

.odd:
    shr ax, 4
    jmp .next_cluster_after

.even:
    and ax, 0x0FFF

.next_cluster_after:
    cmp ax, 0x0FF8
    jae .read_stage_2_finish                 ;; jae = jump above or equal
    ; if this jumps, we are done reading the bootloader_stage_2.bin file

    mov [stage_2_cluster], ax
    jmp .read_stage_2_loop

.read_stage_2_finish:
    ; far jump to the second stage
    mov dl, [ebr_drive_number]              ;; dl = boot device
    mov ax, KERNEL_LOAD_SEGMENT
    mov ds, ax
    mov es, ax

    jmp KERNEL_LOAD_SEGMENT:KERNEL_LOAD_OFFSET

    jmp wait_key_and_reboot                 ;; this should never run

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
    jmp 0FFFFh:0  ;; Jump to beginning of BIOS, thus rebooting 

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
;   - ax: An LBA adress
;   - cl: Number of sectors to read (up to 128)
;   - dl: Drive number
;   - es:bx: Output data
;
disk_read:
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
    pop dx
    pop cx
    pop bx
    pop ax

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

; Messages
msg_hello:
    db "Loading MagnusOS...", ENDLINE, 0x00
msg_floppy_failed:
    db "Floppy disk error!", ENDLINE, 0x00
msg_stage_2_not_found:
    db "STAGE2.BIN file not found!", ENDLINE, 0x00

; Other
stage_2_filename:
    db "STAGE2  BIN"
stage_2_cluster:
    dw 0

KERNEL_LOAD_SEGMENT         equ 0x2000
KERNEL_LOAD_OFFSET          equ 0

times 510-($-$$) db 0
db 0x55, 0xAA

misc_buffer:

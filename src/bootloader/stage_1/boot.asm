bits 16

; Same as \r\n
%define ENDLINE 0x0D, 0x0A
STAGE_2_LOAD_SEGMENT        equ 0x0
STAGE_2_LOAD_OFFSET         equ 0x500

%define fat12 1
%define fat16 2
%define fat32 3

;
; FAT12 header
;

section .fsjump
    jmp short start
    nop

; everything here will be overwritten by mkfs.fat later anyway
section .fsheaders

    bdb_oem:                    times 8 db 0
    bdb_bytes_per_sector:       dw 0
    bdb_sectors_per_cluster:    db 0
    bdb_reserved_sectors:       dw 0
    bdb_fat_count:              db 0
    bdb_dir_entries_count:      dw 0
    bdb_total_sectors:          dw 0
    bdb_media_descriptor_type:  db 0
    bdb_sectors_per_fat:        dw 0
    bdb_sectors_per_track:      dw 0
    bdb_heads:                  dw 0
    bdb_hidden_sectors:         dd 0
    bdb_large_sector_count:     dd 0

    %if (FILESYSTEM == fat32)
        fat32_sectors_per_fat:      dd 0
        fat32_flags:                dw 0
        fat32_fat_version_number:   dw 0
        fat32_rootdir_cluster:      dd 0
        fat32_fsinfo_sector:        dw 0
        fat32_backup_boot_sector:   dw 0
        fat32_reserved:             times 12 db 0
    %endif

    ; extended boot record
    ebr_drive_number:           db 0
                                db 0
    ebr_signature:              db 0
    ebr_volume_id:              db 0x73, 0x65, 0x78, 0x79
    ebr_volume_label:           times 11 db 0
    ebr_system_id:              times 8 db 0

section .entry
    global start
    start:
        ; copy info from mbr for later use
        mov [ebr_drive_number], dl
        add si, 0x8
        mov ebx, [ds:si]
        mov [partition_info.lba], ebx
        add si, 0x4
        mov cl, [ds:si]
        mov [partition_info.size], cl

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
        ; print hello message
        mov si, msg_hello
        call puts

        ; check disk extensions present
        mov ah, 0x41
        mov bx, 0x55AA
        stc
        int 0x13

        jc .no_disk_extensions
        cmp bx, 0xAA55  ;; technically not required
        jne .no_disk_extensions

        ; we know extensions are present
        jmp .after_disk_extensions_check

    .no_disk_extensions:
        mov si, msg_no_extensions
        call puts

        jmp wait_key_and_reboot

    .after_disk_extensions_check:
        ; es:bx = stage 2 load address
        mov ax, STAGE_2_LOAD_SEGMENT
        mov es, ax
        mov bx, STAGE_2_LOAD_OFFSET

        mov dl, [ebr_drive_number]     ; disk id
        mov eax, [stage_2_info.lba]  ;; lba
        mov cl, [stage_2_info.size]   ; length

        call disk_read

    .read_stage_2_finish:
        ; boot device
        mov dl, [ebr_drive_number]
        ; prepare partition location
        mov ebx, [partition_info.lba]
        mov cl, [partition_info.size]

        ; setup stack for stage 2
        mov ax, STAGE_2_LOAD_SEGMENT
        mov ds, ax
        mov es, ax

    global run_stage_2
    run_stage_2:
        ; far jump to the second stage
        jmp STAGE_2_LOAD_SEGMENT:STAGE_2_LOAD_OFFSET

        jmp wait_key_and_reboot                 ;; this should never run

        jmp halt                                ;; safeguard, also should never run

section .text
    disk_error:
        mov si, msg_disk_failed
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
    ; Reads sectors from a disk
    ; - Input parameters:
    ;   - eax: An LBA adress
    ;   - cl: Number of sectors to read
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

        ; here is with extensions
        ; setup dap struct
        mov [extensions_dap.lba], eax
        mov [extensions_dap.segment], es
        mov [extensions_dap.offset], bx
        mov [extensions_dap.sector_count], cl

        mov ah, 0x42
        mov si, extensions_dap
        mov di, 3  ;; 3 retries before failing

    .disk_read_retry:
        pusha

        int 0x13
        jnc .disk_read_done

        ; Failed to read from disk
        popa
        call disk_reset

        dec di
        cmp di, 0
        jne .disk_read_retry
    .disk_read_fail:
        jmp disk_error
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
        jc disk_error
        popa
        ret

section .rodata
    msg_hello:
        db "Loading MagnusOS...", ENDLINE, 0x00
    msg_disk_failed:
        db "Disk error!", ENDLINE, 0x00
    msg_stage_2_not_found:
        db "STAGE2.BIN file not found!", ENDLINE, 0x00
    msg_no_extensions:
        db "No INT-13h extensions!", ENDLINE, 0x00
    stage_2_filename:
        db "STAGE2  BIN"

section .data
    extensions_dap:
        .size           db 0x10
                        db 0
        .sector_count   dw 0
        .offset         dw 0
        .segment        dw 0
        .lba            dq 0

    partition_info:
        .lba            dd 0
        .size           dd 0

    global stage_2_info.lba
    global stage_2_info.size
    stage_2_info:
        .lba            dd 0 ;; why this cant be a double word bewilders, befuddles and discombobulates me, as it only uses 4 bytes. but i do not care to investiagte as to why such an occurence would happen because i believe such efforts would be a waste of time
        .size           db 0

section .bss
    misc_buffer:

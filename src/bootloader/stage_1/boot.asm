bits 16

; Same as \r\n
%define ENDLINE 0x0D, 0x0A
STAGE_2_LOAD_OFFSET         equ 0xC000

%define fat12 1
%define fat16 2
%define fat32 3

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
        mov word [do_not_overwrite], $
        ; es:bx = stage 2 load address
        mov ax, 0 ; always segment 0
        mov es, ax
        mov bx, STAGE_2_LOAD_OFFSET

        mov dl, [ebr_drive_number]     ;; disk id
        mov eax, [stage_2_info.lba]  ;; lba
        mov cx, [stage_2_info.size]   ;; length

    .read_stage_2_loop:
        call disk_read ;; the bios puts number of sectors read in extensions_dap.sector_count
        sub cx, [extensions_dap.sector_count]
        add eax, [extensions_dap.sector_count]

        cmp cx, 0
        jne .read_stage_2_loop

    .read_stage_2_finish:
        ; boot device already in dl

        ; prepare partition location
        mov ebx, [partition_info.lba]
        mov cx, [partition_info.size]

        ; setup stack for stage 2
        mov ax, 0 ; always segment 0
        mov ds, ax  ; will eventually be the stage 2 stack segment (ss register)
        mov es, ax

    global run_stage_2
    run_stage_2:
        ; far jump to the second stage
        jmp 0:STAGE_2_LOAD_OFFSET

        jmp wait_key_and_reboot                 ;; this should never run

        jmp halt                                ;; safeguard, also should never run

section .text
    stage_2_too_big_error:
        mov si, msg_stage_2_too_big
        call puts
        jmp wait_key_and_reboot

    disk_error:
        mov si, msg_disk_failed
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
    ;   - cx: Number of sectors to read
    ;   - dl: Drive number
    ;   - es:bx: Output data
    ;
    ; - Output:
    ;   - extensions_dap.sector_count: Number of sectors successfully read
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
        mov [extensions_dap.sector_count], cx

        mov si, extensions_dap
        mov di, 3  ;; 3 retries before failing

    .disk_read_retry:
        mov ah, 0x42
        int 0x13
        jnc .disk_read_done

        ; Failed to read from disk
        call disk_reset

        dec di
        cmp di, 0
        jne .disk_read_retry
    .disk_read_fail:
        jmp disk_error
    .disk_read_done:

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
    msg_no_extensions:
        db "No INT-13h extensions!", ENDLINE, 0x00
    msg_stage_2_too_big:
        db "Stage 2 is too big!", ENDLINE, 0x00

section .data
    ; for checking that the bootloader stage 2 will not overwrite the disk code and afterwards
    do_not_overwrite:
        dw 0

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
        .lba            dd 0
        .size           dw 0

section .bss
    misc_buffer:

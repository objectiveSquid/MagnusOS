%macro x86_EnterRealMode 0
    [bits 32]
    jmp word 0x18:.16_bit_protected_mode

.16_bit_protected_mode:
    [bits 16]

    ;; disable protected mode bit
    mov eax, cr0
    and al, ~1  ; ~1 = 11111110
    mov cr0, eax

    ;; jump into 16 bit real mode
    jmp word 0x00:.real_mode

.real_mode:
    ;; setup segments
    mov ax, 0
    mov ds, ax
    mov ss, ax

    ;; enable interrupts
    sti
%endmacro

%macro x86_EnterProtectedMode 0
    ;; disable interrupts
    cli

    ;; set protected mode bit
    mov eax, cr0
    or al, 1
    mov cr0, eax

    ;; far jump into protected mode
    jmp dword 0x8:.protected_mode

.protected_mode:
    [bits 32]

    mov ax, 0x10
    mov ds, ax
    mov ss, ax
%endmacro

;
; Convert linear address to segment offset address
; - Input parameters:
;   - 1 ---> linear address
;   - 2 ---> target segment (out) (16 bits)
;   - 3 ---> target 32 bit register to use
;   - 4 ---> target lower 16 bit half of #3 (offset output)
;
%macro ConvertLinearAddress 4
    mov %3, %1
    shr %3, 4
    mov %2, %4
    mov %3, %1
    and %3, 0xF
%endmacro

global x86_OutByte
x86_OutByte:
    mov dx, [esp + 4]
    mov al, [esp + 8]
    out dx, al
    ret

global x86_InByte
x86_InByte:
    mov dx, [esp + 4]
    xor eax, eax
    in al, dx
    ret


;
; uint8_t ASMCALL x86_Disk_GetDriveParams(uint8_t drive, uint16_t *infoFlagsOutput, uint32_t *cylindersOutput, uint32_t *headsOutput, uint32_t *sectorsPerTrackOutput, uint32_t *totalSectorsOutput, uint16_t* bytesPerSectorOutput);
;
global x86_Disk_GetDriveParams
x86_Disk_GetDriveParams:
    [bits 32]

    push ebp
    mov ebp, esp

    x86_EnterRealMode

    push es
    push ds
    push si
    push ebx

    ; setup output buffer address at ds:si = 0x0000:getdriveparams_buffer
    mov ax, 0x0000
    mov ds, ax
    mov si, getdriveparams_buffer

    ; set buffer size (should already be set to 0x001A, but just to be safe ig)
    mov word [getdriveparams_buffer.size], 0x001A  ; 30 bytes

    ; setup remaining registers
    xor eax, eax
    mov ah, 0x48
    mov dl, [bp + 8]  ; Drive number
    int 0x13
    jc .done ; error code already in ah

    ; output info flags
    ConvertLinearAddress [bp + 12], es, ebx, bx
    mov ax, [getdriveparams_buffer.info_flags]
    mov [es:bx], ax

    ; output cylinders
    ConvertLinearAddress [bp + 16], es, ebx, bx
    mov eax, [getdriveparams_buffer.cylinders]
    mov [es:bx], eax

    ; output heads
    ConvertLinearAddress [bp + 20], es, ebx, bx
    mov eax, [getdriveparams_buffer.heads]
    mov [es:bx], eax

    ; output sector per track
    ConvertLinearAddress [bp + 24], es, ebx, bx
    mov eax, [getdriveparams_buffer.sectors_per_track]
    mov [es:bx], eax

    ; output total sectors
    ConvertLinearAddress [bp + 28], es, ebx, bx
    mov eax, [getdriveparams_buffer.total_sectors]
    mov [es:bx], eax

    ; output bytes per sector
    ConvertLinearAddress [bp + 32], es, ebx, bx
    mov ax, [getdriveparams_buffer.bytes_per_sector]
    mov [es:bx], ax

    xor eax, eax ;; if we are here, no error

.done:
    pop ebx
    pop si
    pop ds
    pop es

    push eax

    x86_EnterProtectedMode

    pop eax

    mov esp, ebp
    pop ebp
    ret

; move buffer into function?
getdriveparams_buffer:
    .size               dw 0x001A

    .info_flags         dw 0
    .cylinders          dd 0
    .heads              dd 0
    .sectors_per_track  dd 0
    .total_sectors      dq 0
    .bytes_per_sector   dw 0


;
; bool ASMCALL x86_Disk_Reset(uint8_t drive);
;
global x86_Disk_Reset
x86_Disk_Reset:
    [bits 32]

    push ebp
    mov ebp, esp

    x86_EnterRealMode

    mov ah, 0x00
    mov dl, [bp + 8]    ;; [bp + 8] = drive number
    stc
    int 0x13

    ; returns the error
    mov ax, 1
    sbb ax, 0
    push eax
    x86_EnterProtectedMode
    pop eax

    mov esp, ebp
    pop ebp
    ret


;
; uint8_t ASMCALL x86_Disk_Read(uint8_t drive, uint32_t lba, uint16_t count, uint8_t* readCountOutput, void *dataOutput);
;
global x86_Disk_Read
x86_Disk_Read:
    push ebp
    mov ebp, esp

    x86_EnterRealMode
    
    push ds
    push si
    push es
    push ebx

    mov dl, [bp + 8]    ;; [bp + 8] = drive number

    ; move read count
    mov ax, [bp + 16]   ;; [bp + 16] = count
    mov [extensions_dap.block_count], ax

    ; move output address
    ConvertLinearAddress [bp + 24], es, ebx, bx
    mov [extensions_dap.segment], es
    mov [extensions_dap.offset], bx

    ; move lba
    mov eax, [bp + 12]
    mov [extensions_dap.lba_lower], eax
    xor eax, eax
    mov [extensions_dap.lba_upper], eax

    ; prepare DAP location
    mov ax, 0
    mov ds, ax
    mov ax, extensions_dap
    mov si, ax

    ; actual interrupt
    xor eax, eax
    mov ah, 0x42
    stc
    int 0x13
    mov al, ah
    xor ah, ah
    jc .done

    ; move read count (interrupt updates this to be the actual number of sectors read)
    ConvertLinearAddress [bp + 20], es, ebx, bx
    mov ax, [extensions_dap.block_count]
    mov [es:bx], ax
    
    xor eax, eax ;; if we are here, no error

.done:
    pop ebx
    pop es
    pop si
    pop ds

    push eax
    x86_EnterProtectedMode
    pop eax

    mov esp, ebp
    pop ebp
    ret

; dap = disk address packet
extensions_dap:
    .size               db 0x10
                        db 0
    .block_count        dw 0 ; aka sector_count
    .offset             dw 0
    .segment            dw 0
    .lba_lower          dd 0
    .lba_upper          dd 0

;
; uint8_t x86_VBE_GetControllerInfo(void *controllerInfoOutput);
;
global x86_VBE_GetControllerInfo
x86_VBE_GetControllerInfo:
    push ebp
    mov ebp, esp

    x86_EnterRealMode

    push es
    push edi

    ConvertLinearAddress [bp + 8], es, edi, di
    mov ax, 0x4F00
    int 0x10

    pop edi
    pop es

    push eax
    x86_EnterProtectedMode
    pop eax
    mov al, ah ;; error code

    mov esp, ebp
    pop ebp
    ret

;
; uint8_t ASMCALL x86_VBE_GetModeInfo(uint16_t mode, void *infoOutput);
;
global x86_VBE_GetModeInfo
x86_VBE_GetModeInfo:
    push ebp
    mov ebp, esp

    x86_EnterRealMode

    push es
    push edi
    push cx

    ConvertLinearAddress [bp + 12], es, edi, di
    mov ax, 0x4F01
    mov cx, [bp + 8]
    int 0x10

    pop cx
    pop edi
    pop es

    push eax
    x86_EnterProtectedMode
    pop eax
    mov al, ah ;; error code

    mov esp, ebp
    pop ebp
    ret

;
; uint8_t ASMCALL x86_VBE_SetVideoMode(uint16_t mode);
;
global x86_VBE_SetVideoMode
x86_VBE_SetVideoMode:
    push ebp
    mov ebp, esp

    x86_EnterRealMode

    push bx
    push es
    push edi

    mov ax, 0
    mov es, ax
    mov edi, 0
    mov ax, 0x4F02
    mov bx, [bp + 8]
    int 0x10

    pop edi
    pop es
    pop bx

    push eax
    x86_EnterProtectedMode
    pop eax
    mov al, ah ;; error code

    mov esp, ebp
    pop ebp
    ret

MEMDETECT_GetRegionSignature    equ 0x534D4150  ;; equals to "SMAP"
;
; uint8_t ASMCALL x86_MEMDETECT_GetRegion(MemoryRegion *regionOutput, uint32_t *offset);
;
global x86_MEMDETECT_GetRegion
x86_MEMDETECT_GetRegion:
    push ebp
    mov ebp, esp

    x86_EnterRealMode

    push ebx
    push esi
    push edi
    push es
    push ds

    ; actual interrupt stuff
    ; output buffer
    ConvertLinearAddress [bp + 8], es, edi, di
    ; get target offset
    ConvertLinearAddress [bp + 12], ds, esi, si
    mov ebx, ds:[si]
    ; simples
    mov ecx, 24  ;; output buffer size
    mov edx, MEMDETECT_GetRegionSignature
    mov eax, 0xE820  ;; interrupt code

    ; call interrupt
    int 0x15

    ; error check
    cmp eax, MEMDETECT_GetRegionSignature
    je .carrycheck
    mov eax, 1 ;; unsupported error code
    jmp .done
.carrycheck:
    jnc .errorcheck
    mov eax, 2 ;; carry error code
    jmp .done
.errorcheck:
    cmp ah, 0x86
    jne .move_results
    mov eax, 3 ;; other error code
    jmp .done

.move_results:
    xor eax, eax ;; no error code here
    ; results
    mov ds:[si], ebx  ;; next offset address should still be here

.done:
    pop ds
    pop es
    pop edi
    pop esi
    pop ebx

    push eax ;; error code is put in eax
    x86_EnterProtectedMode
    pop eax

    mov esp, ebp
    pop ebp
    ret

;
; uint32_t ASMCALL x86_MEMDETECT_GetContiguousKBAfter1MB();
;
global x86_MEMDETECT_GetContiguousKBAfter1MB
x86_MEMDETECT_GetContiguousKBAfter1MB:
    push ebp
    mov ebp, esp

    x86_EnterRealMode

    ; do interrupt
    xor eax, eax ;; clear return value
    clc ;; some bioses dont clear carry on success
    mov ah, 0x88
    int 0x15

    ; error checks
    jnc .done ;; on success, skip setting error in eax
    mov eax, 0
    jmp .done ;; no error, result is in ax

.done:
    push eax
    x86_EnterProtectedMode
    pop eax

    mov esp, ebp
    pop ebp
    ret

global x86_Halt
x86_Halt:
    cli
    hlt

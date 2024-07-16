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
;   - 2 ---> target segment (out)
;   - 3 ---> target 32 bit register to use
;   - 4 ---> target lower 16 bit half of #3
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
; bool __attribute__((cdecl)) x86_Disk_GetDriveParams(uint8_t drive, uint8_t *driveTypeOutput, uint16_t *cylindersOutput, uint16_t *headsOutput, uint16_t *sectorsOutput);
;
global x86_Disk_GetDriveParams
x86_Disk_GetDriveParams:
    [bits 32]

    push ebp
    mov ebp, esp

    x86_EnterRealMode

    push es
    push bx
    push esi
    push di

    mov dl, [bp + 8]
    mov di, 0
    mov es, di

    mov ah, 0x8
    stc
    int 0x13

    mov eax, 1
    sbb eax, 0

    ; output values

    ; drive type
    ConvertLinearAddress [bp + 12], es, esi, si
    mov es:[si], bl

    ; cylinders
    mov bl, ch          ;; cylinders - lower bits in ch
    mov bh, cl          ;; cylinders - higher bits in cl
    shr bh, 6
    inc bx

    ConvertLinearAddress [bp + 16], es, esi, si
    mov es:[si], bx

    ; sectors
    xor ch, ch          ;; sectors
    and cl, 0x3F        ;; remove upper 2 bits

    ConvertLinearAddress [bp + 24], es, esi, si
    mov es:[si], cx

    ; heads
    mov cl, dh
    inc cl

    ConvertLinearAddress [bp + 20], es, esi, si
    mov es:[si], cx

    pop di
    pop esi
    pop bx
    pop es

    ; returns the error
    mov eax, 1
    sbb eax, 0
    push eax

    x86_EnterProtectedMode

    ; return bool
    pop eax

    mov esp, ebp
    pop ebp
    ret


;
; bool __attribute__((cdecl)) x86_Disk_Reset(uint8_t drive);
;
global x86_Disk_Reset
x86_Disk_Reset:
    [bits 32]

    push ebp
    mov ebp, esp

    x86_EnterRealMode

    mov ah, 0x00
    mov dl, [bp + 8]    ;; [bp + 4] = drive number
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
; bool __attribute__((cdecl)) x86_Disk_Read(uint8_t drive, uint16_t cylinder, uint16_t head, uint16_t sector, uint8_t count, void __far *dataOutput);
;
global x86_Disk_Read
x86_Disk_Read:
    push ebp
    mov ebp, esp

    x86_EnterRealMode

    push ebx
    push es

    mov dl, [bp + 8]    ;; [bp + 8] = drive number

    mov ch, [bp + 12]    ;; [bp + 12] = cylinder
    mov cl, [bp + 13]
    shl cl, 6

    mov dh, [bp + 16]    ;; [bp + 16] = head

    mov al, [bp + 20]   ;; cl = sector to bits 0-5
    and al, 0x3F
    or cl, al

    mov al, [bp + 24]   ;; [bp + 24] = count

    ConvertLinearAddress [bp + 28], es, ebx, bx

    mov ah, 0x02
    stc
    int 0x13

    mov ax, 1
    sbb ax, 0
    push eax

    x86_EnterProtectedMode

    pop eax

    pop es
    pop ebx

    mov esp, ebp
    pop ebp
    ret

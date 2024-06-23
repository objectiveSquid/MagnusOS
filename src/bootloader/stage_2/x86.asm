bits 16
section _TEXT class=CODE

;
; U4D
;
; Operation:      Unsigned 4 byte divide
; Inputs:         DX;AX   Dividend
;                 CX;BX   Divisor
; Outputs:        DX;AX   Quotient
;                 CX;BX   Remainder
; Volatile:       none
;
global __U4D
__U4D:
    shl edx, 16         ; dx to upper half of edx
    mov dx, ax          ; edx - dividend
    mov eax, edx        ; eax - dividend
    xor edx, edx

    shl ecx, 16         ; cx to upper half of ecx
    mov cx, bx          ; ecx - divisor

    div ecx             ; eax - quot, edx - remainder
    mov ebx, edx
    mov ecx, edx
    shr ecx, 16

    mov edx, eax
    shr edx, 16

    ret

;
; U4M
; Operation:      integer four byte multiply
; Inputs:         DX;AX   integer M1
;                 CX;BX   integer M2
; Outputs:        DX;AX   product
; Volatile:       CX, BX destroyed
;
global __U4M
__U4M:
    shl edx, 16         ; dx to upper half of edx
    mov dx, ax          ; m1 in edx
    mov eax, edx        ; m1 in eax

    shl ecx, 16         ; cx to upper half of ecx
    mov cx, bx          ; m2 in ecx

    mul ecx             ; result in edx:eax (we only need eax)
    mov edx, eax        ; move upper half to dx
    shr edx, 16

    ret

; 
; void _cdecl x86_div64_32(uint64_t dividend, uint32_t divisor, uint64_t *quotientOutput, uint32_t *remainderOutput);
;
global _x86_Math_Div_64_32
_x86_Math_Div_64_32:
    push bp
    mov bp, sp

    push bx

    ; first division
    mov eax, [bp + 8]   ; eax = upper 32 bits of dividend
    mov ecx, [bp + 12]  ; ecx = divisor
    xor edx, edx
    div ecx             ; eax = quotient, edx = remainder

    ; save result from first division
    mov bx, [bp + 16]   ; bx = quotientOutput
    mov [bx + 4], eax

    ; second division
    mov eax, [bp + 4]   ; eax = lower 32 bits of dividend
                        ; edx = old remainder
    div ecx

    ; save result
    mov [bx], eax
    mov bx, [bp + 18]
    mov [bx], edx

    pop bx
    mov sp, bp
    pop bp
    ret

;
; Writes a character in teletype mode with the BIOS
; - Inputs:
;   character (char)
;   page number, must be less than 128 (uint8_t)
;
global _x86_Video_WriteCharTeletype
_x86_Video_WriteCharTeletype:
    push bp
    mov bp, sp

    push bx

    mov ah, 0xE                 ;; [bp + 2] = return offset
    mov al, [bp + 4]            ;; [bp + 4] = character
    mov bh, [bp + 6]            ;; [bp + 6] = page number

                                ;; the parameters are 2 bytes because you cannot push a single byte to the stack
    int 0x10

    pop bx

    mov sp, bp
    pop bp
    ret

;
; Disables interrupts and halts the CPU
;
global _x86_Misc_Halt
_x86_Misc_Halt:
    cli
    hlt

    ret ; this should never run

;
; Reads a character using the BIOS
;
global _x86_Keyboard_ReadChar
_x86_Keyboard_ReadChar:
    push bp
    mov bp, sp

    mov ah, 0x00
    int 0x16        ; al = the input character

    mov sp, bp
    pop bp
    ret

;
; bool _cdecl x86_Disk_Reset(uint8_t drive);
;
global _x86_Disk_Reset
_x86_Disk_Reset:
    push bp
    mov bp, sp

    mov dl, [bp + 4]    ;; [bp + 4] = drive number
    mov ah, 0x00
    int 0x13

    ; returns the error
    mov ax, 1
    sbb ax, 0

    mov sp, bp
    pop bp
    ret

;
; bool _cdecl x86_Disk_Read(uint8_t drive, uint16_t cylinder, uint16_t head, uint16_t sector, uint8_t count, void __far *dataOutput);
;
global _x86_Disk_Read
_x86_Disk_Read:
    push bp
    mov bp, sp

    push bx
    push es

    mov dl, [bp + 4]    ;; [bp + 4] = drive number

    mov ch, [bp + 6]    ;; [bp + 6] = cylinder
    mov cl, [bp + 7]
    shl cl, 6

    mov dh, [bp + 8]    ;; [bp + 8] = head

    mov al, [bp + 10]   ;; cl = sector to bits 0-5
    and al, 0x3F
    or cl, al

    mov al, [bp + 12]   ;; [bp + 12] = count

    mov bx, [bp + 16]   ;; es:bx = far ptr to data output
    mov es, bx
    mov bx, [bp + 14]

    mov ah, 0x02
    stc
    int 0x13

    mov ax, 1
    sbb ax, 0

    pop es
    pop bx

    mov sp, bp
    pop bp
    ret

;
; bool _cdecl x86_Disk_GetDriveParams(uint8_t drive, uint8_t *driveTypeOutput, uint16_t *cylindersOutput, uint16_t *headsOutput, uint16_t *sectorsOutput);
;
global _x86_Disk_GetDriveParams
_x86_Disk_GetDriveParams:
    push bp
    mov bp, sp

    push es
    push bx
    push si
    push di

    mov dl, [bp + 4]
    mov di, 0
    mov es, di

    mov ah, 0x8
    stc
    int 0x13

    ; returns the error
    mov ax, 1
    sbb ax, 0

    ; output values
    mov si, [bp + 6]    ;; drive type
    mov [si], bl

    mov bl, ch          ;; cylinders - lower bits in ch
    mov bh, cl          ;; cylinders - higher bits in cl
    shr bh, 6
    mov si, [bp + 8]
    mov [si], bx

    xor ch, ch          ;; sectors
    and cl, 0x3F        ;; remove upper 2 bits
    mov si, [bp + 12]
    mov [si], cx

    xor cx, cx
    mov cl, dh          ;; heads
    mov si, [bp + 10]
    mov [si], cx

    pop di
    pop si
    pop bx
    pop es

    mov sp, bp
    pop bp
    ret

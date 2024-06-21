bits 16
section _TEXT class=CODE

; 
; void _cdecl x86_div64_32(uint64_t dividend, uint32_t divisor, uint64_t *quotientOutput, uint32_t *remainderOutput);
;
global _x86_div64_32
_x86_div64_32:
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

    ; by the _cdecl calling standards, the ax/eax register should be saved by the caller
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

global _x86_halt
_x86_halt:
    cli
    hlt

    ret ; this should never run

global _x86_Keyboard_ReadChar
_x86_Keyboard_ReadChar:
    push bp
    mov bp, sp

    mov ah, 0x00
    int 0x16        ; al = the input character

    mov sp, bp
    pop bp
    ret
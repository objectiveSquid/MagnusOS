%include "lib/x86/cpumodes.inc"
%include "lib/x86/address.inc"

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

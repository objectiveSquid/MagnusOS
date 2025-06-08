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

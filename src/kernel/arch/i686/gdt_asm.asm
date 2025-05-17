[bits 32]

;
; void __attribute__((cdecl)) i686_GDT_Load(GDTDescriptor *descriptor, uint16_t codeSegment, uint16_t dataSegment);
;
global i686_GDT_Load
i686_GDT_Load:
    push ebp
    mov ebp, esp

    ; load gdt
    mov eax, [ebp + 8] ;; descriptor
    lgdt [eax]

    ; reload code segment
    mov eax, [ebp + 12] ;; codeSegment
    push eax
    push .reload_code_segment
    retf

.reload_code_segment:

    mov ax, [ebp + 16] ;; dataSegment
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax

    mov esp, ebp
    pop ebp
    ret
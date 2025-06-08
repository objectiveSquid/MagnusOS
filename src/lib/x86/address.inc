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

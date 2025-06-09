ENTRY(entry)
OUTPUT_FORMAT("binary")
phys = 0x0000C000;

SECTIONS {
    . = phys;

    .entry              : { __entry_start  = .; *(.entry  ) }

    /* this is to put 16 bit code first, so that it is under 0xFFFF */
    .asm : {
        KEEP(*entry.o)
        KEEP(util/*x86.o)
    }

    .text               : { __text_start   = .; *(.text   ) }
    .data               : { __data_start   = .; *(.data   ) }
    .rodata             : { __rodata_start = .; *(.rodata ) }
    .bss                : { __bss_start    = .; *(.bss    ) }

    __end = .;
}

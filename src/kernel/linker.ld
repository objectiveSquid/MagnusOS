ENTRY(start)
OUTPUT_FORMAT("elf32-i386")

PHDRS {
  text PT_LOAD FLAGS (PF_R | PF_X);
  rodata PT_LOAD FLAGS (PF_R);
  data PT_LOAD FLAGS (PF_R | PF_W);
}

SECTIONS {
    .text               : { __text_start   = .; *(.text   ) } : text
    .data               : { __data_start   = .; *(.data   ) } : text
    .rodata             : { __rodata_start = .; *(.rodata ) } : rodata
    .bss                : { __bss_start    = .; *(.bss    ) } : data
    __bss_stop = .;

    __end = .;
}

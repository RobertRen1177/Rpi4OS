SECTIONS{
    . = 0x80000 /* the kernel loads code into this address */
    .text : {KEEP(*(.text_boot)) *(.text .text.* .gnu.linkonce.t*)}
    .rodata : {*(.rodata .rodata.* .gnu.linkonce.r*)}
    PROVIDE(_data = .) /*if _data does not exist set the value equal to the current address*/
    .data : { *(.data .data.* .gnu.linkonce.d*) }
    .bss (NOLOAD) : {
        . = ALIGN(16); /*forces . to be 16-byte aligned*/
        __bss_start = .; /*the start of the bss section is the current address in memory we currently are*/
        *(.bss .bss.*) 
        *(COMMON)
        __bss_end = .; /*the end of the __bss_section is the current address in memory we currently are*/
    } 
    _end = .;

   /DISCARD/ : { *(.comment) *(.gnu*) *(.note*) *(.eh_frame*) }
}
__bss_size = (__bss_end - __bss_start)>>3;


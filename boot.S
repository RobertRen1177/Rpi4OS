.section ".text.boot" // in my linker script in linker.ld, this goes at the very front at address 0x80000 because thatis

.global _start

_start:
    mrs x1, mpidr_el1
    and x1, x1, #3
    cbz x1, 2f

1: 
    wfe
    b 1b
2:
    // checking to see if currently in El1
    mrs     x1, CurrentEL
    lsr     x1, x1, #2
    cmp     x1, #2
    b.ne    5f // if we are continue to clearing out the BSS

    // bring kernel into EL1
    ldr     x1, =0x30D00800         
    msr     sctlr_el1, x1
    mov     x1, #(1 << 31)          
    msr     hcr_el2, x1
    mov     x1, #0x3c5             
    msr     spsr_el2, x1
    adr     x1, 5f
    msr     elr_el2, x1
    eret

5:
    // stack pointer set up and clearing the .bss section of memory
    ldr     x1, =_start
    mov     sp, x1

    ldr     x1, =__bss_start
    ldr     w2, =__bss_size 
3:  cbz     w2, 4f
    str     xzr, [x1], #8
    sub     w2, w2, #1
    cbnz    w2, 3b

4:  bl      main
    b       1b
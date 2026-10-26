#pragma once

enum {
    PERIPHERAL_BASE = 0xFE000000,
    GPFSEL0         = PERIPHERAL_BASE + 0x200000,
    GPSET0          = PERIPHERAL_BASE + 0x20001C,
    GPCLR0          = PERIPHERAL_BASE + 0x200028,
    GPPUPPDN0       = PERIPHERAL_BASE + 0x2000E4
};

enum {
    GPIO_MAX_PIN       = 58,
    GPIO_FUNCTION_ALT5 = 2,
};

enum {
    Pull_None = 0,
};

// mini UART (UART1) registers, shared with the keyboard interrupt handler
enum {
    AUX_BASE        = PERIPHERAL_BASE + 0x215000,
    AUX_IRQ_REG     = AUX_BASE + 0,  // bit 0 set while the mini UART has an interrupt pending
    AUX_ENABLES     = AUX_BASE + 4,
    AUX_MU_IO_REG   = AUX_BASE + 64,
    AUX_MU_IER_REG  = AUX_BASE + 68,
    AUX_MU_IIR_REG  = AUX_BASE + 72,
    AUX_MU_LCR_REG  = AUX_BASE + 76,
    AUX_MU_MCR_REG  = AUX_BASE + 80,
    AUX_MU_LSR_REG  = AUX_BASE + 84,
    AUX_MU_CNTL_REG = AUX_BASE + 96,
    AUX_MU_BAUD_REG = AUX_BASE + 104,
    AUX_UART_CLOCK  = 500000000,
    UART_MAX_QUEUE  = 16 * 1024
};

// addresses are unsigned long because pointers are 64 bits on the rpi4
void mmio_write(unsigned long reg, unsigned int val);
unsigned int mmio_read(unsigned long reg);
void uart_init();
void writeByte(char ch);
void writeString(const char* buffer);

#pragma once

enum {
    PERIPHERAL_BASE = 0xFE000000,
    GPFSEL0         = PERIPHERAL_BASE + 0x200000,
    GPSET0          = PERIPHERAL_BASE + 0x20001C,
    GPCLR0          = PERIPHERAL_BASE + 0x200028,
    GPPUPPDN0       = PERIPHERAL_BASE + 0x2000E4
};

enum {
    GPIO_MAX_PIN       = 58, // BCM2711 has 58 GPIO pins, numbered 0-57
    GPIO_FUNCTION_ALT5 = 2,
};

enum {
    Pull_None = 0,
};

// addresses are unsigned long because pointers are 64 bits on the rpi4
void mmio_write(unsigned long reg, unsigned int val);
unsigned int mmio_read(unsigned long reg);
void uart_init();
void writeByte(char ch);
void writeString(const char* buffer);

#include "IO/include/io.h"
// the raspberry pi exposes its peripherals (so it's UART controllers and shit like that as memory addresses)
// to write and read to UART, you need to know the memory address that is writing to the UART perpherial and the read memory
// of the UART
void mmio_write(unsigned long reg, unsigned int val) {
    * (volatile unsigned int*) reg = val; // Volatile means the compiler cannot just assume the value of the variable
}
unsigned int mmio_read(unsigned long reg) {
    unsigned int return_val = * ((volatile unsigned int *) reg);
    return return_val; 
}

// to write to a pin, like we mentioned before, we must write to a spot in memory that was chosen by the rpi4 manufacturers
// the commands and muxing for each of the pins lives in memory
unsigned int gpio_call(unsigned int pin_number, unsigned int value, unsigned int base, unsigned int field_size,
                        unsigned int field_max) { 
    /*
      - each register is 4 bytes long which is 32 bit
      - each pin is represented by "field_size" bits
      - to modify a certain pin, you first need to find what register the pin belongs too, and then 
        the specific bits within that register that actually model what the bit does
    */
    unsigned int pins_per_register = 32 / field_size;
    if (pin_number >= field_max) {
        return 0; // this is an error
    }
    
    int pin_register = pin_number / pins_per_register; 
    int pin_bits = pin_number % pins_per_register;
    unsigned int shift = (pin_bits * field_size);
    unsigned int mask = ((1 << field_size) - 1) << shift;

    // actually writing over
    unsigned long reg = base + (pin_register * 4); // memory address of the 4 byte register
    unsigned int curval = mmio_read(reg); // read what is currently there
    curval &= ~(mask); // zero out the 3 bits that we want to write over
    curval |= ((value << shift) & mask); // writing the new values that we have
    mmio_write(reg, curval); //actually writing into memory
    return 1; 
}

unsigned int gpio_set     (unsigned int pin_number, unsigned int value) { return gpio_call(pin_number, value, GPSET0, 1, GPIO_MAX_PIN); }
unsigned int gpio_clear   (unsigned int pin_number, unsigned int value) { return gpio_call(pin_number, value, GPCLR0, 1, GPIO_MAX_PIN); }
unsigned int gpio_pull    (unsigned int pin_number, unsigned int value) { return gpio_call(pin_number, value, GPPUPPDN0, 2, GPIO_MAX_PIN); }
unsigned int gpio_function(unsigned int pin_number, unsigned int value) { return gpio_call(pin_number, value, GPFSEL0, 3, GPIO_MAX_PIN); }

void gpio_useAsAlt5(unsigned int pin_number) {
    gpio_pull(pin_number, Pull_None);
    gpio_function(pin_number, GPIO_FUNCTION_ALT5);
}

// initalizing UART these are just the settings we want
enum {
    AUX_BASE        = PERIPHERAL_BASE + 0x215000,
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

#define AUX_MU_BAUD(baud) ((AUX_UART_CLOCK/(baud*8))-1)

void uart_init() {
    mmio_write(AUX_ENABLES, 1); //enable UART1
    mmio_write(AUX_MU_IER_REG, 0);
    mmio_write(AUX_MU_CNTL_REG, 0);
    mmio_write(AUX_MU_LCR_REG, 3); //8 bits
    mmio_write(AUX_MU_MCR_REG, 0);
    mmio_write(AUX_MU_IER_REG, 0);
    mmio_write(AUX_MU_IIR_REG, 0xC6); //disable interrupts
    mmio_write(AUX_MU_BAUD_REG, AUX_MU_BAUD(115200));
    gpio_useAsAlt5(14);
    gpio_useAsAlt5(15);
    mmio_write(AUX_MU_CNTL_REG, 3); //enable RX/TX
}

// bit 5 of LSR is set when the transmitter can accept a byte
unsigned int uart_isWriteByteReady() { return mmio_read(AUX_MU_LSR_REG) & 0x20; }

void writeByte(char ch) {
    while (!uart_isWriteByteReady()) {
        continue;
    }
    mmio_write(AUX_MU_IO_REG, ch);
    
}

void writeString(const char* buffer) {
    const char* curr = buffer; 
    while (*curr != '\0') {
        writeByte(*curr); 
        curr += 1; 
    }
}
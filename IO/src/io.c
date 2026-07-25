#include "include/io.h"

// the raspberry pi exposes its peripherals (so it's UART controllers and shit like that as memory addresses)
// to write to UART, you need 
void mmio_write(long reg, unsigned int val) {
    * (volatile unsigned int*) reg = val;
}
unsigned int mmio_read(long reg) {
    int return_val = * ((volatile unsigned int *) reg);
    return return_val; 
}
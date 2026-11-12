#include "IO/include/io.h"
#include "interrupts/include/interrupt.h"

void main() {
    uart_init();
    writeString("hello world\r\n");

    irq_init();     // vectors, GIC, UART RX interrupt (must come after uart_init)
    timer_init();   // arm system timer compare 1 and 3
    irq_enable();

    // demo: each timer prints once a second, and we echo whatever keys come in
    while (1) {
        irq_wait();                    // sleep until an IRQ queues some work
        while (pop_irq() != -1) {}     // run the deferred handlers

        int key;
        while ((key = keyboard_read()) != KEY_NONE) {
            switch (key) {
                case KEY_UP:    writeString("[UP]");    break;
                case KEY_DOWN:  writeString("[DOWN]");  break;
                case KEY_LEFT:  writeString("[LEFT]");  break;
                case KEY_RIGHT: writeString("[RIGHT]"); break;
                case '\r':      writeString("\r\n");    break;
                default:
                    writeString("[");
                    writeByte(key);
                    writeString("]");
                    break;
            }
        }
    }
}

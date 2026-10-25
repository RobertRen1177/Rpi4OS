#include "IO/include/io.h"
void main() {
    uart_init();
    writeString("hello world\r\n");
}
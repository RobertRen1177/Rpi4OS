#include <stdio.h>
#include "IO/include/io.h"
void main() {
    uart_init();
    uart_write_buffer("hello world");
}
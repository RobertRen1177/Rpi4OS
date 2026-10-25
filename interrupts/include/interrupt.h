#pragma once 

#define NUM_INTERRUPTS 64

// what an __attribute__() is it is a special flag you can put on a function which then tells your compiler (gcc, clang, etc.)
// to compile the code in assembly in a different way
// for example, when it sees that the function is an interrupt, it will add assembly instructions to save our frame
// and then use the iret command instead of the regular ret assembly command to return 

// PERIPHERAL COMMUNICATION

// How an interrupt works in ARM
void __attribute__(interrupt("SWI")) software_interrupt(void);

void __attribute__(interrupt("IRQ")) interrupt_handler(void);


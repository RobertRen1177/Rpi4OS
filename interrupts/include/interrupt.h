#pragma once 

#include "IO/include/io.h"

#define NUM_INTERRUPTS 64
#define CLOCKHZ 1000000

/*
This is going to be a lengthy explanaton of the timer:
How the timer works is there is the counter_lo and the counter_hi together form a 64 bit integer
<counter_hi><counter_lo>
these basically count the amount of times the counter is incrementing

compare: is an array of length 4
each element of the array can be programmed to be an interrupt

so when counter_lo = compare[i] -> then it will send an interrupt to timer_i
the reason why we do this is so we can create multiple timer interrupts

After the interrupt, you need to update the value further into the future in order to make sure
the timer interrupts again. 

control status tells you which timer has fired
*/
// both sizes must be powers of 2 so indices can wrap with a mask
// (the buffers themselves live in interrupt.c)
#define IRQ_BUFFER_SIZE 512
#define RX_BUF_SIZE 256

struct timer_regs {
    volatile unsigned int control_status;
    volatile unsigned int counter_lo;
    volatile unsigned int counter_hi;
    volatile unsigned int compare[4];
};
// this is where the timer shi is located in memory
#define REGS_TIMER ((struct timer_regs *)(PERIPHERAL_BASE + 0x00003000))
#define TIMER_1_HZ 250
#define TIMER_3_HZ 50
enum timer_cs {
    TIMER_CS_M0 = 1,
    TIMER_CS_M1 = 2,
    TIMER_CS_M2 = 4,
    TIMER_CS_M3 = 8
};

// these are the interrupt controller peripherals
struct gic400_dist_regs {    
    volatile unsigned int ctl;
    volatile unsigned int type;
    volatile unsigned int iid;
    volatile unsigned int res0[29];
    volatile unsigned int group[32];          // 0x080: one bit per interrupt
    volatile unsigned int set_enable[32];     // 0x100: one bit per interrupt, write 1 to enable
    volatile unsigned int clear_enable[32];   // 0x180: one bit per interrupt, write 1 to disable
    volatile unsigned int set_pending[32];    // 0x200
    volatile unsigned int clear_pending[32];  // 0x280
    volatile unsigned int set_active[32];     // 0x300
    volatile unsigned int clear_active[32];   // 0x380
    volatile unsigned char priority[1020];    // 0x400: one byte per interrupt
    volatile unsigned int res1;
    volatile unsigned char target[1020];      // 0x800: one byte per interrupt, one bit per core
};

struct gic400_cpu_regs {
    volatile unsigned int ctl;                
    volatile unsigned int priority_mask;
    volatile unsigned int binary_point;
    volatile unsigned int interrupt_ack;
    volatile unsigned int end_of_interrupt;
};

#define REGS_GICD ((struct gic400_dist_regs *)0xFF841000)
#define REGS_GICC ((struct gic400_cpu_regs *)0xFF842000)

enum vc_irqs {
    SYS_TIMER_IRQ_1 = 96 + 1,
    SYS_TIMER_IRQ_3 = 96 + 3,
    AUX_IRQ         = 96 + 29   // mini UART (shares the line with the SPIs, but we only use the UART)
};

#define GIC_SPURIOUS_IRQ 1023
#define VIRT_TIMER_IRQ 27

// keyboard_read() returns a plain ASCII character, or one of these for the arrow keys
// (a terminal sends arrows as the escape sequence ESC [ A..D, which we decode for you)
enum keys {
    KEY_NONE  = -1,
    KEY_UP    = 0x100,
    KEY_DOWN  = 0x101,
    KEY_RIGHT = 0x102,
    KEY_LEFT  = 0x103
};

// initialization
void irq_init();
void timer_init();
void irq_enable();
void irq_disable();
void irq_init_vectors();
void irq_wait();

// irq buffer interface
void irq_handler();
int push_irq(unsigned int irq);
int pop_irq();

// timer  hard irqs and soft irqs
void timer_hardirq(unsigned int irq);
void timer_handler(unsigned int irq);

// Keyboard UART hardirq and handler
// this is not put into the IRQ queue due to userspace needed to read
void uart_hardirq();
int uart_handler();
int keyboard_read();

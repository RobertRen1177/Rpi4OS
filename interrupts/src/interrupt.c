#include "Memory/include/mem.h"
#include "interrupts/include/interrupt.h"


static volatile unsigned int irq_buffer[IRQ_BUFFER_SIZE];
static volatile unsigned int irq_head = 0;
static volatile unsigned int irq_tail = 0;
static volatile unsigned int irq_dropped = 0;

static volatile char rx_buf[RX_BUF_SIZE];
static volatile unsigned int rx_head = 0, rx_tail = 0;
static volatile unsigned int rx_dropped = 0;
static volatile unsigned int rx_overruns = 0;

static unsigned int timer_1_ticks = 0;
static unsigned int timer_3_ticks = 0;

void irq_handler() {
    while (1) {
        unsigned int ack = REGS_GICC->interrupt_ack;
        unsigned int irq = ack & 0x3ffu;
        if (irq == GIC_SPURIOUS_IRQ) {
            break;
        }
        if (irq == SYS_TIMER_IRQ_1 || irq == SYS_TIMER_IRQ_3) {
            timer_hardirq(irq);
            push_irq(irq);
        } else if (irq == AUX_IRQ) {
            uart_hardirq();
            push_irq(irq);
        } else {
            REGS_GICD->clear_enable[irq / 32] = 1u << (irq % 32);
        }
        REGS_GICC->end_of_interrupt = ack;
    }
}

int push_irq(unsigned int irq) {
    unsigned int tail = irq_tail;
    if (tail - irq_head == IRQ_BUFFER_SIZE) {
        irq_dropped++;
        return -1; 
    }
    irq_buffer[tail & (IRQ_BUFFER_SIZE - 1)] = irq; 
    irq_tail = tail + 1;
    return 0;
}

void timer_hardirq(unsigned int irq) {
    if (irq == SYS_TIMER_IRQ_1) {
        REGS_TIMER->compare[1] += CLOCKHZ / TIMER_1_HZ;
        REGS_TIMER->control_status = TIMER_CS_M1;
    }
    if (irq == SYS_TIMER_IRQ_3) {
        REGS_TIMER->compare[3] += CLOCKHZ / TIMER_3_HZ;
        REGS_TIMER->control_status = TIMER_CS_M3;
    }
}

void uart_hardirq() {
    unsigned int lsr;
    while ((lsr = mmio_read(AUX_MU_LSR_REG)) & 1) {
        if (lsr & 2) {
            rx_overruns++; // the 8-byte hardware FIFO overflowed before we got here
        }
        char ch = (char)mmio_read(AUX_MU_IO_REG);
        unsigned int tail = rx_tail;
        if (tail - rx_head < RX_BUF_SIZE) {
            rx_buf[tail & (RX_BUF_SIZE - 1)] = ch;
            rx_tail = tail + 1;
        } else {
            rx_dropped++;
        }
    }
}


int pop_irq() {
    unsigned int head = irq_head;
    if (head == irq_tail) {
        return -1;
    }
    unsigned int handler_irq = irq_buffer[head & (IRQ_BUFFER_SIZE - 1)]; 
    irq_head = head + 1; 

    if (handler_irq == SYS_TIMER_IRQ_1 || handler_irq == SYS_TIMER_IRQ_3) {
        timer_handler(handler_irq);
    }
    return (int)handler_irq;
}

void timer_handler(unsigned int irq) {
    if (irq == SYS_TIMER_IRQ_1 && ++timer_1_ticks % TIMER_1_HZ == 0) {
        writeString("<timer1 1s>");
    }
    if (irq == SYS_TIMER_IRQ_3 && ++timer_3_ticks % TIMER_3_HZ == 0) {
        writeString("<timer3 1s>");
    }
}

int uart_handler() {
    unsigned int head = rx_head;
    if (rx_tail == head) {
        return KEY_NONE;
    }
    unsigned char ch = rx_buf[head & (RX_BUF_SIZE - 1)];
    rx_head = head + 1;
    return ch;
}

int keyboard_read() {
    static int esc_state = 0; 
    int ch;
    while ((ch = uart_handler()) != KEY_NONE) {
        if (esc_state == 0) {
            if (ch == 0x1b) { 
                esc_state = 1; 
                continue; 
            }
            return ch;
        }
        if (esc_state == 1) {
            esc_state = (ch == '[') ? 2 : 0;
            if (esc_state == 2) continue;
            return ch;
        }
        esc_state = 0;
        switch (ch) {
            case 'A': return KEY_UP;
            case 'B': return KEY_DOWN;
            case 'C': return KEY_RIGHT;
            case 'D': return KEY_LEFT;
            default:  return ch;
        }
    }
    return KEY_NONE;
}

void irq_wait() {
    irq_disable();
    if (irq_head == irq_tail) {
        __asm__ volatile("wfi");
    }
    irq_enable();
}

// ---------- setup ----------
static void gic_enable_line(unsigned int irq) {
    REGS_GICD->priority[irq] = 0xA0;
    REGS_GICD->target[irq] = 1; 
    REGS_GICD->set_enable[irq / 32] = 1u << (irq % 32);
}

void timer_init() {
    unsigned int now = REGS_TIMER->counter_lo;
    REGS_TIMER->compare[1] = now + CLOCKHZ / TIMER_1_HZ;
    REGS_TIMER->compare[3] = now + CLOCKHZ / TIMER_3_HZ;
}

void irq_init() {
    irq_init_vectors();

    REGS_GICD->ctl = 0;
    gic_enable_line(SYS_TIMER_IRQ_1);
    gic_enable_line(SYS_TIMER_IRQ_3);
    gic_enable_line(AUX_IRQ);
    REGS_GICD->ctl = 1;

    REGS_GICC->priority_mask = 0xFF;
    REGS_GICC->ctl = 1;
    mmio_write(AUX_MU_IER_REG, 0xD);
}

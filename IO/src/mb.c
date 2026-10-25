#include "IO/include/mb.h"
enum {
    VIDEOCORE_MBOX = (PERIPHERAL_BASE + 0x0000B880),
    MBOX_READ      = (VIDEOCORE_MBOX + 0x0),
    MBOX_POLL      = (VIDEOCORE_MBOX + 0x10),
    MBOX_SENDER    = (VIDEOCORE_MBOX + 0x14),
    MBOX_STATUS    = (VIDEOCORE_MBOX + 0x18),
    MBOX_CONFIG    = (VIDEOCORE_MBOX + 0x1C),
    MBOX_WRITE     = (VIDEOCORE_MBOX + 0x20),
    MBOX_RESPONSE  = 0x80000000,
    MBOX_FULL      = 0x80000000,
    MBOX_EMPTY     = 0x40000000
};

volatile unsigned int __attribute__((aligned(16))) mbox[36];

unsigned int parse_message_data(unsigned int message) {
    unsigned int mask = 0xFFFFFFF0; // top 28 bits are the data
    return message & mask; 
}

unsigned int parse_message_status(unsigned int message) {
    unsigned int mask = 0x0000000F; // low 4 bits are the channel
    return message & mask; 
}


void write_mailbox(unsigned int value) {
    while (mmio_read(MBOX_STATUS) & MBOX_FULL) ;
    mmio_write(MBOX_WRITE, value);
}

unsigned int read_mailbox() {
    while (mmio_read(MBOX_STATUS) & MBOX_EMPTY);
    return mmio_read(MBOX_READ);
}

unsigned int write_to_mailbox(unsigned char channel) {
    // the address/pointer of the message we want to send to the GPU
    unsigned int r = (unsigned int) ((long) &mbox & ~(0xF)) | (channel & 0xF);
    write_mailbox(r);

    while (1) {
        while (mmio_read(MBOX_STATUS) & MBOX_EMPTY);
        if (r == mmio_read(MBOX_READ)) {
            return mbox[1]==MBOX_RESPONSE;
        }
    }
    return 0;
}
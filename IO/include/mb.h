// the mailbox is how the raspberry pi CPU and GPU communicate with each other
// a mailbox message request is 4 bytes

// mbox is a buffer in memory
// to talk with the GPU, we basically write the address of our buffer
// into the memory perpherial of the GPU (which is pre definied in firmware)
// the GPU will then read this from its own inbox, and then it will go to the place in memory

#include "io.h"
extern volatile unsigned int mbox[36];

enum {
    MBOX_REQUEST  = 0
};

enum {
    MBOX_CH_POWER = 0,
    MBOX_CH_FB    = 1,
    MBOX_CH_VUART = 2,
    MBOX_CH_VCHIQ = 3,
    MBOX_CH_LEDS  = 4,
    MBOX_CH_BTNS  = 5,
    MBOX_CH_TOUCH = 6,
    MBOX_CH_COUNT = 7,
    MBOX_CH_PROP  = 8 // Request from ARM for response by VideoCore
};

enum {
    MBOX_TAG_SETPOWER   = 0x28001,
    MBOX_TAG_SETCLKRATE = 0x38002,
    MBOX_TAG_SETPHYWH   = 0x48003,
    MBOX_TAG_SETVIRTWH  = 0x48004,
    MBOX_TAG_SETVIRTOFF = 0x48009,
    MBOX_TAG_SETDEPTH   = 0x48005,
    MBOX_TAG_SETPXLORDR = 0x48006,
    MBOX_TAG_GETFB      = 0x40001,
    MBOX_TAG_GETPITCH   = 0x40008,
    MBOX_TAG_LAST       = 0
};

unsigned int read_mailbox();
void write_mailbox(unsigned int value); 
unsigned int write_to_mailbox(unsigned char channel);
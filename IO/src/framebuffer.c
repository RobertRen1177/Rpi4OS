#include "IO/include/framebuffer.h"
// when talking to the mailbox it follows this request:

/*
 basically we have a buffer in memory called mbox
 when we want to send a message to the GPU we can fill our mbox array with status bits
 these are basically messages/ "hardcoded signals that firmware looks for that maps to different functions/request for the GPU"
 We then put the address of this buffer into the mailbox RX
 The GPU then reads this message and looks at the memory address
 By looking through the bits and seeing the signals, it then knows what to do

*/

// filled in by init_mbox once the GPU hands us a framebuffer
static unsigned int width, height, pitch, isrgb;
static unsigned char *fb;

void init_mbox() {
    mbox[0] = 35*4; // Length of message in bytes
    mbox[1] = MBOX_REQUEST;

    mbox[2] = MBOX_TAG_SETPHYWH; // Tag identifier
    mbox[3] = 8; // Value size in bytes
    mbox[4] = 0;
    mbox[5] = 1920; // Value(width)
    mbox[6] = 1080; // Value(height)

    mbox[7] = MBOX_TAG_SETVIRTWH;
    mbox[8] = 8;
    mbox[9] = 8;
    mbox[10] = 1920;
    mbox[11] = 1080;

    mbox[12] = MBOX_TAG_SETVIRTOFF;
    mbox[13] = 8;
    mbox[14] = 8;
    mbox[15] = 0; // Value(x)
    mbox[16] = 0; // Value(y)

    mbox[17] = MBOX_TAG_SETDEPTH;
    mbox[18] = 4;
    mbox[19] = 4;
    mbox[20] = 32; // Bits per pixel

    mbox[21] = MBOX_TAG_SETPXLORDR;
    mbox[22] = 4;
    mbox[23] = 4;
    mbox[24] = 0; // BGR - our palette words are 0xRRGGBB, so blue must sit in byte 0.
                  // (Old firmware ignored this tag and gave us BGR regardless;
                  // current firmware honours it, so ask for what we mean!)

    mbox[25] = MBOX_TAG_GETFB;
    mbox[26] = 8;
    mbox[27] = 8;
    mbox[28] = 4096; // FrameBufferInfo.pointer
    mbox[29] = 0;    // FrameBufferInfo.size

    mbox[30] = MBOX_TAG_GETPITCH;
    mbox[31] = 4;
    mbox[32] = 4;
    mbox[33] = 0; // Bytes per line

    mbox[34] = MBOX_TAG_LAST;

    // we now need to package this request and send it 
    
    // Check call is successful and we have a pointer with depth 32
    if (write_to_mailbox(MBOX_CH_PROP) && mbox[20] == 32 && mbox[28] != 0) {
        mbox[28] &= 0x3FFFFFFF; // Convert GPU address to ARM address
        width = mbox[10];       // Actual virtual width
        height = mbox[11];      // Actual virtual height
        pitch = mbox[33];       // Number of bytes per line
        isrgb = mbox[24];       // Pixel order
        fb = (unsigned char *)((long)mbox[28]);
    }
}

void draw_pixel(struct point p, unsigned int color) {
    if (fb == 0 || p.x < 0 || p.y < 0 || (unsigned int) p.x >= width || (unsigned int) p.y >= height) {
        return;
    }
    unsigned long offset = (unsigned long) p.y * pitch + (unsigned long) p.x * 4;
    *((volatile unsigned int *)(fb + offset)) = color;
}

void draw_line(struct point start, struct point end, unsigned int color) {
    int dx = end.x > start.x ? end.x - start.x : start.x - end.x;
    int dy = end.y > start.y ? start.y - end.y : end.y - start.y;
    int sx = start.x < end.x ? 1 : -1;
    int sy = start.y < end.y ? 1 : -1;
    int err = dx + dy;

    struct point p = start;
    while (1) {
        draw_pixel(p, color);
        if (p.x == end.x && p.y == end.y) {
            break;
        }
        int e2 = 2 * err;
        if (e2 >= dy) {
            err += dy;
            p.x += sx;
        }
        if (e2 <= dx) {
            err += dx;
            p.y += sy;
        }
    }
}

void draw_square(struct point c1, struct point c2, struct point c3, struct point c4, unsigned int color) {
    draw_line(c1, c2, color);
    draw_line(c2, c3, color);
    draw_line(c3, c4, color);
    draw_line(c4, c1, color);
}

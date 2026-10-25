#pragma once

// the frame buffer is a specific module / channel that our GPU can write messages to
// this is how we can draw messages on our screen!
#include "IO/include/mb.h"
struct point {
    int x;
    int y;
};

void init_mbox();
void draw_pixel(struct point p, unsigned int color);
void draw_line(struct point start, struct point end, unsigned int color);
void draw_square(struct point c1, struct point c2, struct point c3, struct point c4, unsigned int color);

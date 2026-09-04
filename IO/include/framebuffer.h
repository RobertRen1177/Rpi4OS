// the frame buffer is a specific module / channel that our GPU can write messages to
// this is how we can draw messages on our screen!
#include "IO/include/mb.h"
struct point {
    int x;
    int y;
};

void init_mbox();
void draw_line(struct point start, struct point end);
void draw_square(struct point c1, struct point c2, struct point c3, struct point c4);

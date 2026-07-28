// the mailbox is how the raspberry pi CPU and GPU communicate with each other
// a mailbox message request is 4 bytes
#include "io.h"

unsigned int read_mailbox();
void write_mailbox(unsigned int value); 
unsigned int write_to_mailbox(unsigned char channel);
#include "unbcj.h"

void unbcj16(unsigned char __far *buf, unsigned int len) {
    unsigned int i = 0;
    if (len < 3) return;
    while (i + 3 <= len) {
        unsigned char op = buf[i];
        if (op == 0xE8 || op == 0xE9) {
            unsigned int v = buf[i + 1] | ((unsigned int)buf[i + 2] << 8);
            v = (unsigned int)(v - (i + 3)) & 0xFFFFu;
            buf[i + 1] = (unsigned char)(v & 0xFF);
            buf[i + 2] = (unsigned char)((v >> 8) & 0xFF);
            i += 3;
        } else {
            i += 1;
        }
    }
}

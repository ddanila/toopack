#include "ovly.h"
#include "unlzsa2.h"
#include "unbcj.h"
#include "dos.h"

unsigned char __far *ovly_load(const unsigned char *packed,
                               unsigned int raw_size) {
    unsigned int paragraphs = (unsigned int)((raw_size + 15u) >> 4);
    unsigned int seg = dos_alloc(paragraphs);
    unsigned char __far *dst;

    if (seg == 0) return (unsigned char __far *)0;

    dst = (unsigned char __far *)((unsigned long)seg << 16);
    unlzsa2(packed, dst);
    unbcj16(dst, raw_size);
    return dst;
}

#include "../../src/dos.h"
#include "../../src/unlzsa2.h"
#include "../../src/unbcj.h"
#include "sample_packed.h"

int main(void) {
    unsigned int seg = dos_alloc(4);
    unsigned char __far *dst;
    if (seg == 0) { dos_puts("alloc fail\r\n$"); return 1; }
    dst = (unsigned char __far *)((unsigned long)seg << 16);
    unlzsa2(sample_packed, dst);
    unbcj16(dst, SAMPLE_RAW_SIZE);
    /* Read first byte of decoded output and print message */
    if (dst[0] == 0xE8) {
        dos_puts("byte0 = E8 (expected)\r\n$");
    } else {
        dos_puts("byte0 != E8 (mismatch)\r\n$");
    }
    return 0;
}

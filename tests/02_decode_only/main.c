/* 02_decode_only: alloc + LZSA2 decode + BCJ-inverse, no compare.
 * Smaller cousin of 03_decode; useful for diagnosing the decoder in
 * isolation. Prints first 8 bytes of decoded output. */

#include "../../src/dos.h"
#include "../../src/unlzsa2.h"
#include "../../src/unbcj.h"
#include "sample_packed.h"

int main(void) {
    unsigned int seg = dos_alloc(4);
    unsigned char __far *dst;
    unsigned int dec_size;
    unsigned int i;

    if (seg == 0) { dos_puts("alloc fail\r\n$"); return 1; }
    dst = (unsigned char __far *)((unsigned long)seg << 16);

    dec_size = unlzsa2(sample_packed, dst);
    unbcj16(dst, dec_size);

    dos_puts("dec_size=$");
    dos_puthex16(dec_size);
    dos_puts(", first 8 bytes:$");
    for (i = 0; i < 8; ++i) {
        dos_putc(' ');
        dos_puthex8(dst[i]);
    }
    dos_puts("\r\n$");

    return 0;
}

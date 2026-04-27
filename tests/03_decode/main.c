/* 03_decode: round-trip test for the runtime decoder + BCJ + dispatcher.
 *
 * Embeds a packed overlay (LZSA2 + BCJ-forward applied at build time)
 * and the original raw bytes, decodes via ovly_load(), and compares.
 */

#include "../../src/ovly.h"
#include "../../src/dos.h"

#include "sample_packed.h"
#include "sample_raw.h"

int main(void) {
    unsigned char __far *got;
    unsigned int i;

    got = ovly_load(sample_packed, SAMPLE_RAW_SIZE);
    if (got == 0) {
        dos_puts("alloc fail\r\n$");
        return 2;
    }

    for (i = 0; i < SAMPLE_RAW_SIZE; ++i) {
        if (got[i] != sample_raw_bytes[i]) {
            dos_puts("fail\r\n$");
            return 1;
        }
    }
    dos_puts("ok\r\n$");
    return 0;
}

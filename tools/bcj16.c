/* Host-side 16-bit x86 BCJ E8/E9 filter.
 *
 * Forward (encode):  for each byte E8 or E9, treat the next 2 bytes as a
 *                    little-endian rel16 and replace with the blob-relative
 *                    target offset (i + 3 + rel16) mod 2^16. Lets LZ77
 *                    match the same call site at different positions.
 *
 * Inverse (decode):  rel16 = (encoded - (i + 3)) mod 2^16. Restores the
 *                    original instruction stream.
 *
 * Forward and inverse are mathematical inverses, so the round-trip is
 * lossless even when an E8/E9 byte was actually part of an immediate or
 * data (false positive). False positives only cost some ratio.
 *
 * Notes:
 *  - Designed for raw flat 8086 modules, not for executables with their
 *    own load offsets. Caller is responsible for ensuring "i" matches the
 *    runtime offset the inverse pass will see.
 *  - This is the host-side reference. The runtime decoder lives in
 *    src/unbcj.* and implements only the inverse pass.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int filter(int decode) {
    size_t cap = 65536, len = 0;
    unsigned char *buf = (unsigned char *)malloc(cap);
    if (!buf) return 1;

    int c;
    while ((c = getchar()) != EOF) {
        if (len == cap) {
            cap *= 2;
            unsigned char *nb = (unsigned char *)realloc(buf, cap);
            if (!nb) { free(buf); return 1; }
            buf = nb;
        }
        buf[len++] = (unsigned char)c;
    }

    if (len >= 3) {
        size_t i = 0;
        while (i + 3 <= len) {
            unsigned char op = buf[i];
            if (op == 0xE8 || op == 0xE9) {
                unsigned int v = buf[i + 1] | (buf[i + 2] << 8);
                unsigned int delta = (unsigned int)(i + 3);
                if (decode) {
                    v = (v - delta) & 0xFFFF;
                } else {
                    v = (v + delta) & 0xFFFF;
                }
                buf[i + 1] = (unsigned char)(v & 0xFF);
                buf[i + 2] = (unsigned char)((v >> 8) & 0xFF);
                i += 3;
            } else {
                i += 1;
            }
        }
    }

    if (fwrite(buf, 1, len, stdout) != len) { free(buf); return 1; }
    free(buf);
    return 0;
}

int main(int argc, char **argv) {
    int decode = 0;
    if (argc == 2) {
        if (!strcmp(argv[1], "-e")) decode = 0;
        else if (!strcmp(argv[1], "-d")) decode = 1;
        else { fprintf(stderr, "usage: bcj16 -e|-d < in > out\n"); return 1; }
    } else if (argc != 1) {
        fprintf(stderr, "usage: bcj16 -e|-d < in > out\n");
        return 1;
    }
    return filter(decode);
}

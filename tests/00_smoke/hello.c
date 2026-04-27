/* 00_smoke: prove the toolchain (Watcom + custom CRT + kvikdos) works. */

#include "../../src/dos.h"

int main(void) {
    dos_puts("hello from toopack\r\n$");
    return 0;
}

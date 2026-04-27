/* 05_cmdloop: stdin-driven dispatcher.
 *   h -> help  (cold)
 *   a -> about (cold)
 *   q -> quit
 *   any other char (incl. \r, \n) -> ignored silently */

#include "../../src/dos.h"
#include "../../src/ovly.h"
#include "cold_packed.h"

typedef void (__far __cdecl *cold_fn)(unsigned int cmd);

int main(void) {
    unsigned char __far *blob;
    cold_fn cold;
    unsigned int c;

    blob = ovly_load(cold_packed, COLD_RAW_SIZE);
    if (blob == 0) {
        dos_puts("(overlay alloc failed)\r\n$");
        return 1;
    }
    cold = (cold_fn)blob;

    dos_puts("ready\r\n$");
    for (;;) {
        c = dos_getc();
        if (c == 'h' || c == 'H') {
            cold(0);
        } else if (c == 'a' || c == 'A') {
            cold(1);
        } else if (c == 'q' || c == 'Q') {
            dos_puts("bye\r\n$");
            return 0;
        }
        /* anything else silently ignored */
    }
}

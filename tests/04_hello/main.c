/* 04_hello: prove a packed cold-overlay function can be unpacked at
 * runtime and far-called. main prints "hello " (always-resident);
 * the cold overlay prints "world\r\n" (decoded on-the-fly). */

#include "../../src/dos.h"
#include "../../src/ovly.h"
#include "cold_packed.h"

typedef void (__far __cdecl *overlay_fn)(void);

int main(void) {
    unsigned char __far *blob;
    overlay_fn cold;

    dos_puts("hello $");

    blob = ovly_load(cold_packed, COLD_RAW_SIZE);
    if (blob == 0) {
        dos_puts("(overlay alloc failed)\r\n$");
        return 1;
    }

    cold = (overlay_fn)blob;
    (*cold)();
    return 0;
}

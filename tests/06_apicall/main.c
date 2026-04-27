/* 06_apicall: cold overlay calls back into resident via a struct of
 * function pointers (toopack_api_t). Demonstrates the cold ↔ resident
 * ABI documented in NOTES.md. */

#include "../../src/dos.h"
#include "../../src/api.h"
#include "../../src/ovly.h"
#include "cold_packed.h"

/* Read CS at runtime — Watcom can't fold our segment into a static far
 * pointer initializer in a tiny .COM (no relocation table), so we patch
 * the api struct's segment fields manually. */
extern unsigned int get_cs(void);
#pragma aux get_cs = "mov ax, cs" value [ax];

/* Declare same-named labels as data so we can take their offsets via
 * `&X` without Watcom trying to bake in a segment relocation (the .COM
 * format has no relocation table). The actual implementations are
 * __far __cdecl (retf) in src/api.asm. */
extern char rapi_puts_addr;
#pragma aux rapi_puts_addr "_rapi_puts";
extern char rapi_puthex16_addr;
#pragma aux rapi_puthex16_addr "_rapi_puthex16";

typedef void (__far __cdecl *cold_fn)(const toopack_api_t __far *api,
                                       unsigned int value);

int main(void) {
    static toopack_api_t api;
    unsigned int cs;
    unsigned int *fp;
    unsigned char __far *blob;
    cold_fn cold;

    cs = get_cs();
    fp = (unsigned int *)&api;
    fp[0] = (unsigned int)&rapi_puts_addr;     fp[1] = cs;   /* api.puts     */
    fp[2] = (unsigned int)&rapi_puthex16_addr; fp[3] = cs;   /* api.puthex16 */

    dos_puts("calling cold\r\n$");

    blob = ovly_load(cold_packed, COLD_RAW_SIZE);
    if (blob == 0) {
        dos_puts("alloc fail\r\n$");
        return 1;
    }
    cold = (cold_fn)blob;
    cold(&api, 0xCAFE);

    dos_puts("done\r\n$");
    return 0;
}

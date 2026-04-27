/* Resident-side API exposed to cold overlays.
 *
 * Cold modules receive a `const toopack_api_t __far *api` as their first
 * arg and call helpers via the function pointers. The struct lives in
 * the resident's data segment; cold's far calls land in the resident's
 * code segment with DS=cold's-segment, so each helper saves/restores DS
 * if it needs the resident's own data.
 *
 * See tests/06_apicall for the canonical wiring.
 */

#ifndef TOOPACK_API_H
#define TOOPACK_API_H

typedef struct {
    void (__far __cdecl *puts)(const char __far *s);     /* INT 21h AH=09h */
    void (__far __cdecl *puthex16)(unsigned int v);      /* 4 hex digits   */
} toopack_api_t;

extern void __far __cdecl rapi_puts(const char __far *s);
extern void __far __cdecl rapi_puthex16(unsigned int v);

#endif

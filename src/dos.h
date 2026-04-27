/* Minimal DOS int 21h wrappers. No libc dependency.
 *
 * All functions use Watcom inline-asm + #pragma aux for tight code.
 */

#ifndef TOOPACK_DOS_H
#define TOOPACK_DOS_H

/* Print a $-terminated string via INT 21h AH=09h.
 * String must be in DS (near pointer is fine in tiny model). */
extern void dos_puts(const char *s);
#pragma aux dos_puts =          \
    "mov ah, 09h"               \
    "int 21h"                   \
    parm [dx]                   \
    modify [ax];

/* Write one byte via INT 21h AH=02h (DL = char). */
extern void dos_putc(char c);
#pragma aux dos_putc =          \
    "mov ah, 02h"               \
    "int 21h"                   \
    parm [dl]                   \
    modify [ax];

/* Read one byte from stdin via INT 21h AH=08h (no echo). Returns the
 * byte zero-extended into AX. */
extern unsigned int dos_getc(void);
#pragma aux dos_getc =          \
    "mov ah, 08h"               \
    "int 21h"                   \
    "xor ah, ah"                \
    value [ax]                  \
    modify [ax];

/* Allocate a DOS memory block. INT 21h AH=48h: BX = paragraphs.
 * Returns segment on success, 0 on failure. */
extern unsigned int dos_alloc(unsigned int paragraphs);
#pragma aux dos_alloc =         \
    "mov ah, 48h"               \
    "int 21h"                   \
    "jnc skip"                  \
    "xor ax, ax"                \
    "skip:"                     \
    parm [bx]                   \
    value [ax]                  \
    modify [ax];

/* Free a previously allocated DOS memory block. ES = segment. */
extern void dos_free(unsigned int seg);
#pragma aux dos_free =          \
    "mov es, bx"                \
    "mov ah, 49h"               \
    "int 21h"                   \
    parm [bx]                   \
    modify [ax es];

/* Print one byte as 2 hex digits via dos_putc. */
static void dos_puthex8(unsigned char v) {
    static const char H[] = "0123456789ABCDEF";
    dos_putc(H[(v >> 4) & 0x0F]);
    dos_putc(H[v & 0x0F]);
}

/* Print 16-bit value as 4 hex digits. */
static void dos_puthex16(unsigned int v) {
    dos_puthex8((unsigned char)(v >> 8));
    dos_puthex8((unsigned char)(v & 0xFF));
}

#endif

/* LZSA2 raw-block decoder, calling into the upstream 8088 asm.
 *
 * Source layout assumed:
 *   src:  resident, near pointer (DS = our DGROUP)
 *   dst:  any segment, far pointer
 *
 * The asm clobbers DS during match copies (it temporarily sets DS = ES
 * to read back-references from the output buffer) but restores it.
 */

#ifndef TOOPACK_UNLZSA2_H
#define TOOPACK_UNLZSA2_H

/* Backing routine — implemented in unlzsa2.S, no name decoration. */
extern void lzsa2_decompress(void);
#pragma aux lzsa2_decompress "*"        \
    modify [ax bx cx dx si di bp es];

/* Wrapper: presents a clean C signature; pragma below inlines a single
 * `call lzsa2_decompress` with parameters loaded into the right regs. */
extern unsigned int unlzsa2(const unsigned char *src,
                            unsigned char __far *dst);
/* The decoder uses BP internally (`xor bp, bp` then accumulates a 32-bit
 * match offset into it) and does not restore it. We save/restore BP here
 * so callers can rely on stack frames surviving the call. */
#pragma aux unlzsa2 =                   \
    "push bp"                           \
    "call lzsa2_decompress"             \
    "pop bp"                            \
    parm [si] [es di]                   \
    value [ax]                          \
    modify [si di ax bx cx dx es];

#endif

/* Overlay loader: allocates a DOS memory block, decompresses a packed
 * overlay into it, runs the BCJ inverse pass. Returns a far pointer to
 * the unpacked overlay (typically the entry point), or NULL on failure.
 *
 * Caller owns the lifetime: pass the segment back to dos_free() when
 * done, or just leak it for single-shot programs.
 */

#ifndef TOOPACK_OVLY_H
#define TOOPACK_OVLY_H

unsigned char __far *ovly_load(const unsigned char *packed,
                               unsigned int raw_size);

#endif

/* BCJ E8/E9 inverse pass for 16-bit x86, operates on a far buffer.
 * Restores original rel16 displacements from blob-offset-encoded form. */

#ifndef TOOPACK_UNBCJ_H
#define TOOPACK_UNBCJ_H

void unbcj16(unsigned char __far *buf, unsigned int len);

#endif

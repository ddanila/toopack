; Wasm wrapper exposing the upstream LZSA2 8088 decoder as a public symbol.
; The decoder bytes themselves are pre-assembled by tools/gen_decoder_inc.sh
; and included verbatim. By emitting them through wasm into the same _TEXT
; segment that start.asm opens with `org 100h`, the linker fixes up the
; symbol consistently with the rest of our code.

        .8086

_TEXT   segment word public 'CODE'

        public  lzsa2_decompress
lzsa2_decompress:
        INCLUDE unlzsa2.inc

_TEXT   ends
        end

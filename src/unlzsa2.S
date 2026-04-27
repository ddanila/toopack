; Re-export the upstream LZSA2 8088 decoder as a globally-visible symbol
; that Watcom can link against.
;
; The 0x100 zero pad mimics Watcom's `org 100h` so wlink's symbol
; offsets line up after segment merge. Without this, the call to
; lzsa2_decompress is fixed up 0x100 bytes too low.

   bits 16
   segment _LZSA_TEXT public class=CODE
   global lzsa2_decompress
   %include "decompress_small_v2.S"

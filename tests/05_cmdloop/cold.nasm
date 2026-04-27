; Cold overlay with dispatched entry: one entry point, two messages.
;
; void __far __cdecl cold(unsigned int cmd);
;   cmd == 0 -> print help
;   cmd != 0 -> print about
;
; Stack layout after `push bp; mov bp,sp` (each slot 2 bytes):
;   [bp+0] saved bp
;   [bp+2] return IP
;   [bp+4] return CS
;   [bp+6] cmd arg

bits 16
org 0

cold_entry:
        push    bp
        mov     bp, sp
        push    ds
        push    cs
        pop     ds
        mov     ax, [bp+6]
        or      ax, ax
        jnz     do_about
        mov     dx, msg_help
        jmp     do_print
do_about:
        mov     dx, msg_about
do_print:
        mov     ah, 09h
        int     21h
        pop     ds
        pop     bp
        retf

msg_help:  db "commands: h, a, q", 13, 10, "$"
msg_about: db "toopack overlay demo", 13, 10, "$"

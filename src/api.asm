; Resident-side API helpers callable from cold modules.
;
; All entries are __far __cdecl. Watcom 16-bit decorates __cdecl symbols
; with a leading underscore.
;
; Stack frame after `push bp; mov bp, sp` on a __far __cdecl call:
;     [bp+0]  saved_bp
;     [bp+2]  return IP
;     [bp+4]  return CS
;     [bp+6]  arg0 low word    (for far ptr: offset; for int: value)
;     [bp+8]  arg0 high word   (for far ptr: segment)
;     ...
;
; Helpers save and restore DS because callers (cold) have DS pointing at
; the cold segment, not at our DGROUP.

        .8086

_TEXT   segment word public 'CODE'
        assume  cs:DGROUP, ds:DGROUP

        public  _rapi_puts
        public  _rapi_puthex16

; void __far __cdecl rapi_puts(const char __far *s);
_rapi_puts:
        push    bp
        mov     bp, sp
        push    ds
        lds     dx, [bp+6]      ; DS:DX = s (far)
        mov     ah, 9           ; INT 21h AH=09h: print $-terminated string
        int     21h
        pop     ds
        pop     bp
        retf

; void __far __cdecl rapi_puthex16(unsigned int v);
; Prints v as 4 uppercase hex digits via INT 21h AH=02h. Doesn't read
; resident data — INT 21h AH=02h takes only DL, so DS doesn't matter.
_rapi_puthex16:
        push    bp
        mov     bp, sp
        push    bx
        push    cx
        push    dx
        mov     bx, [bp+6]
        mov     cx, 4
hex_loop:
        rol     bx, 1
        rol     bx, 1
        rol     bx, 1
        rol     bx, 1
        mov     al, bl
        and     al, 0Fh
        add     al, '0'
        cmp     al, '9'+1
        jb      hex_emit
        add     al, 'A'-'0'-10
hex_emit:
        mov     dl, al
        mov     ah, 2
        int     21h
        loop    hex_loop
        pop     dx
        pop     cx
        pop     bx
        pop     bp
        retf

_TEXT   ends
        end

; Cold overlay: prints "world\r\n" via INT 21h AH=09h, then far-returns.
;
; Conventions imposed on cold modules (see NOTES.md):
;   - Entry at offset 0 of the unpacked blob.
;   - Caller far-calls in (CS = unpacked segment).
;   - We get to assume nothing about DS; we set DS = CS, restore on exit.
;   - Returns via RETF.
;   - May only use INT services (no libc, no resident-helper callback yet).

bits 16
org 0

cold_entry:
        push    ds
        push    cs
        pop     ds              ; ds = our (the unpacked) segment
        mov     dx, msg
        mov     ah, 09h
        int     21h
        pop     ds
        retf

msg:    db      "world", 13, 10, "$"

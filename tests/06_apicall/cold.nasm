; Cold overlay that calls back into the resident via toopack_api_t.
;
; void __far __cdecl cold(const toopack_api_t __far *api, unsigned int value);
;   stack on entry (after push bp; mov bp,sp):
;     [bp+0]  saved_bp
;     [bp+2]  ret IP
;     [bp+4]  ret CS
;     [bp+6]  api offset
;     [bp+8]  api segment
;     [bp+10] value
;
; api struct layout (from src/api.h):
;     +0  puts     (4 bytes, far)
;     +4  puthex16 (4 bytes, far)

bits 16
org 0

cold_entry:
        push    bp
        mov     bp, sp
        push    ds
        push    cs
        pop     ds              ; DS = cold segment, for our msg_* labels

        ; api->puts(msg_prefix)
        push    cs              ; far ptr seg = our segment
        mov     ax, msg_prefix
        push    ax              ; far ptr off
        les     bx, [bp+6]      ; ES:BX = api
        call    far [es:bx + 0]
        add     sp, 4

        ; api->puthex16(value)
        push    word [bp+10]
        les     bx, [bp+6]
        call    far [es:bx + 4]
        add     sp, 2

        ; api->puts(msg_suffix)
        push    cs
        mov     ax, msg_suffix
        push    ax
        les     bx, [bp+6]
        call    far [es:bx + 0]
        add     sp, 4

        pop     ds
        pop     bp
        retf

msg_prefix: db "from cold: $"
msg_suffix: db 13, 10, "$"

; Minimal tiny-model .COM startup.
; No libc, no heap, no argv. Calls main_, then DOS exit.
; Watcom default calling convention is __watcall: int return is in AX.

        .8086

DGROUP  group   _TEXT, CONST, CONST2, _DATA, _BSS

_TEXT   segment word public 'CODE'
        assume  cs:DGROUP, ds:DGROUP, ss:DGROUP, es:DGROUP

        org     100h            ; .COM load address

        public  _cstart_
        extrn   main_:near

_cstart_:
        call    main_
        mov     ah, 4Ch         ; DOS exit, AL = main's return value
        int     21h

_TEXT   ends

CONST   segment word public 'DATA'
CONST   ends
CONST2  segment word public 'DATA'
CONST2  ends
_DATA   segment word public 'DATA'
_DATA   ends
_BSS    segment word public 'BSS'
_BSS    ends

        end     _cstart_

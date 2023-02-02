use32
    org 0x1000000
    db  'MENUET01'
    dd  1, start, i_end, e_end, e_end, 0, 0

__DEBUG__ = 1
__DEBUG_LEVEL__ = 1

include 'proc32.inc'
include 'macros.inc'
include 'debug-fdo.inc'

EFLAGS.ID = 1 SHL 21

start:
        pushfd
        btr     dword[esp], BSF EFLAGS.ID
        popfd

        mcall   68, 11

        mcall   12, 1
        mcall   0, <50,200>, <50,100>, 0x3488ffff, , window_title
        mcall   12, 2

        mcall   15, 1, 2, 2
        mcall   15, 4, 1
        mcall   15, 6
        mov     ecx, 0
        mov     edx, 0xff
        mov     [eax+0], cl
        mov     [eax+1], cl
        mov     [eax+2], cl
        mov     [eax+3], dl
        mov     [eax+4], dl
        mov     [eax+5], dl
        mov     [eax+6], dl
        mov     [eax+7], dl
        mov     [eax+8], dl
        mov     [eax+9], cl
        mov     [eax+10], cl
        mov     [eax+11], cl
        mcall   15, 7, eax
        mcall   15, 3

redraw:
        mcall   12, 1
        mcall   0, <50,200>, <50,100>, 0x3488ffff, , window_title
        mcall   12, 2

still:
        mcall   10
        dec     eax
        jz      redraw
        dec     eax
        jz      key
button:
        mcall   17
        shr     eax, 8

        cmp     eax, 1
        jz      exit

        jmp     still

key:
        mcall   2
        cmp     ah, 0x09        ; TAB key

        jmp     still

exit:
        mcall   -1

;include_debug_strings

window_title db 'loader',0

i_end:

rb 0x100        ; stack
e_end:

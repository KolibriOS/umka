use32
    org 0
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

redraw:
        mcall   12, 1
        mcall   0, <100,200>, <100,100>, 0x34ff8888, , window_title
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

window_title db 'just a window',0

i_end:

rb 0x100        ; stack
e_end:

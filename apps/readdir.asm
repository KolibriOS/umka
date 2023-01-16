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

        DEBUGF 1, "abcde\n"
        mcall   70, fs70
        DEBUGF 1, "files in dir: %d\n", ebx
        mcall   -1
exit:
;        mcall   18, 9, 2
        mcall   -1
        mcall   -2      ; just to check it's unreachable

include_debug_strings
fs70:
.sf     dd 1
        dd 0
        dd 0
        dd 42
        dd dir_buf
        db 0
        dd dir_name

dir_name db '/hd0/1/',0
i_end:
dir_buf:
rb 0x10000

rb 0x100        ; stack
e_end:

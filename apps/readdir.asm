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

        mcall   12, 1
        mcall   0, <100,200>, <100,100>, 0x34888888, , window_title
        mcall   12, 2

        DEBUGF 1, "abcde\n"
        mcall   70, fs70
        DEBUGF 1, "files in dir: %d\n", ebx

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

        mcall   18, 19, 4, 0
.next:
        mcall   37, 0
        add     eax, 0x00010001
        mov     edx, eax
        mcall   18, 19, 4
        mcall   5, 10
;        mov     ecx, 0x1000000
;        loopnz  $
        jmp     .next
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
window_title db 'readdir test',0

i_end:
dir_buf:
rb 0x10000

rb 0x100        ; stack
e_end:

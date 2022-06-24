use32
    org 0
    db  'MENUET01'
    dd  1, start, i_end, e_end, e_end, 0, 0

__DEBUG__ = 1
__DEBUG_LEVEL__ = 1

include 'proc32.inc'
include 'macros.inc'
include 'debug-fdo.inc'

start:
        DEBUGF 1, "Hello board\n"
exit:
        mcall   18, 9, 2
        mcall   -1
        mcall   -2      ; just to check it's unreachable

include_debug_strings
i_end:
rb 0x100        ;stack
e_end:

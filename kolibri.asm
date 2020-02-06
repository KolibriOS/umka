format ELF

__DEBUG__ = 1
__DEBUG_LEVEL__ = 1

extrn malloc
malloc fix __malloc
_malloc fix malloc
extrn free
free fix __free
_free fix free

sys_msg_board equ _sys_msg_board
cli equ nop
iretd equ retd

lang fix en
preboot_blogesc = 0       ; start immediately after bootlog
pci_code_sel = 0
VESA_1_2_VIDEO  = 0      ; enable vesa 1.2 bank switch functions

purge mov,add,sub
purge mov,add,sub
section '.text' executable align 32

public i40

coverage_begin:
public coverage_begin

include 'macros.inc'
macro diff16 blah1,blah2,blah3 {
}
include 'proc32.inc'
include 'struct.inc'
include 'const.inc'
;OS_BASE      = os_base
LFB_BASE     = lfb_base
SLOT_BASE    = os_base + 0x00080000
window_data  = os_base + 0x00001000
WIN_STACK    = os_base + 0x0000C000
WIN_POS      = os_base + 0x0000C400
KEY_COUNT    = os_base + 0x0000F400
KEY_BUFF     = os_base + 0x0000F401 ; 120*2 + 2*2 = 244 bytes, actually 255 bytes
BTN_COUNT    = os_base + 0x0000F500
BTN_BUFF     = os_base + 0x0000F501
BTN_ADDR     = os_base + 0x0000FE88
TASK_BASE    = os_base + 0x00003010
CURRENT_TASK = os_base + 0x00003000
TASK_COUNT   = os_base + 0x00003004
TASK_BASE    = os_base + 0x00003010

include 'system.inc'
include 'fdo.inc'
; block and fs
include 'blkdev/disk.inc'
include 'blkdev/disk_cache.inc'
include 'fs/fs_lfn.inc'
include 'crc.inc'
; video
include 'core/dll.inc'
include 'video/framebuffer.inc'
include 'video/vesa20.inc'
include 'video/vga.inc'
include 'video/cursors.inc'
include 'unpacker.inc'
include 'gui/window.inc'
include 'gui/button.inc'
load_file equ __load_file
include 'gui/skincode.inc'
restore load_file
include 'gui/draw.inc'
include 'gui/font.inc'
include 'core/syscall.inc'
 
include 'sha3.asm'

struct VDISK
  File     dd ?
  SectCnt  DQ ?
  SectSize dd ?  ; sector size
ends

public sha3_256_oneshot as 'hash_oneshot'
proc sha3_256_oneshot c uses ebx esi edi ebp, _ctx, _data, _len
        stdcall sha3_256.oneshot, [_ctx], [_data], [_len]
        ret
endp

align 4
syscall_getpixel:                       ; GetPixel
        mov     ecx, [_display.width]
        xor     edx, edx
        mov     eax, ebx
        div     ecx
        mov     ebx, edx
        xchg    eax, ebx
        and     ecx, 0xFBFFFFFF  ;negate 0x04000000 use mouseunder area
        call    dword [GETPIXEL]; eax - x, ebx - y
        mov     [esp + 32], ecx
        ret

align 4
calculate_fast_getting_offset_for_WinMapAddress:
; calculate data area for fast getting offset to _WinMapAddress
        xor     eax, eax
        mov     ecx, [_display.height]
        mov     edi, d_width_calc_area
        cld
@@:
        stosd
        add     eax, [_display.width]
        dec     ecx
        jnz     @r
        ret
;------------------------------------------------------------------------------
align 4
calculate_fast_getting_offset_for_LFB:
; calculate data area for fast getting offset to LFB
        xor     eax, eax
        mov     ecx, [_display.height]
        mov     edi, BPSLine_calc_area
        cld
@@:
        stosd
        add     eax, [_display.lfb_pitch]
        dec     ecx
        jnz     @r
redrawscreen:
force_redraw_background:
        ret
 
; TODO: move to trace_lbr
public set_eflags_tf
set_eflags_tf:
        pushfd
        pop     eax
        mov     ecx, [esp + 4]
        and     ecx, 1
        shl     ecx, 8  ; TF
        and     eax, NOT (1 SHL 8)
        or      eax, ecx
        push    eax
        popfd
        ret

; TODO: move to trace_lwp
public get_lwp_event_size
get_lwp_event_size:
        mov     eax, 0x80000001
        cpuid
        bt      ecx, 15
        jnc     .no_lwp
        mov     eax, 0x8000001c
        cpuid
        and     eax, 1001b
        cmp     eax, 1001b
        jnz     .no_lwp
        movzx   eax, bh
        ret
  .no_lwp:
        xor     eax, eax
        ret


public kos_getcwd
proc kos_getcwd c uses ebx esi edi ebp, _buf, _len
        mov     eax, 30
        mov     ebx, 2
        mov     ecx, [_buf]
        mov     edx, [_len]
        call    sys_current_directory
        ret
endp

public kos_cd
proc kos_cd c uses ebx esi edi ebp, _path
        mov     eax, 30
        mov     ebx, 1
        mov     ecx, [_path]
        call    sys_current_directory
        ret
endp

public kos_time_to_epoch
proc kos_time_to_epoch c uses ebx esi edi ebp, _time
        mov     esi, [_time]
        call    fsCalculateTime
        add     eax, 978307200  ; epoch to 2001.01.01
        ret
endp


public kos_init
proc kos_init c uses ebx esi edi ebp
        MEMORY_BYTES = 128 SHL 20
        DISPLAY_WIDTH = 400
        DISPLAY_HEIGHT = 300
        mov     [pg_data.mem_amount], MEMORY_BYTES
        mov     [pg_data.pages_count], MEMORY_BYTES / PAGE_SIZE
        mov     [pg_data.pages_free], MEMORY_BYTES / PAGE_SIZE

        mov     dword[sysdir_name], 'sys'
        mov     dword[sysdir_path], 'HD0/'
        mov     word[sysdir_path+4], '1'

        mov     eax, SLOT_BASE + 2*256
        mov     [current_slot], eax
        mov     word[cur_dir.path], '/'
        mov     [eax+APPDATA.cur_dir], cur_dir
        mov     [eax+APPDATA.wnd_clientbox.left], 0
        mov     [eax+APPDATA.wnd_clientbox.top], 0
        mov     dword[CURRENT_TASK], 2
        mov     dword[TASK_COUNT], 2
        mov     dword[TASK_BASE], CURRENT_TASK + 2*sizeof.TASKDATA

        mov     [_display.x], 0
        mov     [_display.y], 0
        mov     [_display.width], DISPLAY_WIDTH
        mov     [_display.lfb_pitch], DISPLAY_WIDTH*4
        mov     [_display.height], DISPLAY_HEIGHT
        mov     [PUTPIXEL], Vesa20_putpixel32
        mov     [GETPIXEL], Vesa20_getpixel32
        mov     [_display.bytes_per_pixel], 4
        mov     [_display.bits_per_pixel], 32
        mov     [MOUSE_PICTURE], dword mousepointer
        call    calculate_fast_getting_offset_for_WinMapAddress
        call    calculate_fast_getting_offset_for_LFB
        mov     [X_UNDER], 500
        mov     [Y_UNDER], 500
        mov     word[MOUSE_X], 40
        mov     word[MOUSE_Y], 30

        mov     eax, [_display.width]
        mul     [_display.height]
        mov     [_display.win_map_size], eax

        mov     [_display.check_mouse], check_mouse_area_for_putpixel_new
        mov     [_display.check_m_pixel], check_mouse_area_for_getpixel_new

        stdcall kernel_alloc, eax
        mov     [_display.win_map], eax

        mov     [BgrDrawMode], eax
        mov     [BgrDataWidth], eax
        mov     [BgrDataHeight], eax
        mov     [mem_BACKGROUND], 4
        mov     [img_background], static_background_data

        ; from set_variables
        xor     eax, eax
        mov     [BTN_ADDR], dword BUTTON_INFO ; address of button list
        mov     byte [KEY_COUNT], al              ; keyboard buffer
        mov     byte [BTN_COUNT], al              ; button buffer

        call    set_window_defaults
        mov     [skin_data], 0
        call    load_default_skin

        ret
endp

window_title db 'hello',0

public kos_disk_add
proc kos_disk_add c uses ebx esi edi ebp, _file_name, _disk_name
        extrn cio_disk_init
        ccall   cio_disk_init, [_file_name]
        stdcall disk_add, vdisk_functions, [_disk_name], eax, DISK_NO_INSERT_NOTIFICATION
        push    eax
        stdcall disk_media_changed, eax, 1
        pop     edx
        movi    ecx, 1
        mov     esi, [edx+DISK.Partitions]
.next_part:
        cmp     ecx, [edx+DISK.NumPartitions]
        ja      .part_done
        DEBUGF 1, "/%s/%d: ", [edx+DISK.Name], ecx
        lodsd
        inc     ecx
        cmp     [eax+PARTITION.FSUserFunctions], xfs._.user_functions
        jnz     @f
        DEBUGF 1, "xfs\n"
        jmp     .next_part
@@:
        cmp     [eax+PARTITION.FSUserFunctions], ext_user_functions
        jnz     @f
        DEBUGF 1, "ext\n"
        jmp     .next_part
@@:
        cmp     [eax+PARTITION.FSUserFunctions], fat_user_functions
        jnz     @f
        DEBUGF 1, "fat\n"
        jmp     .next_part
@@:
        cmp     [eax+PARTITION.FSUserFunctions], ntfs_user_functions
        jnz     @f
        DEBUGF 1, "ntfs\n"
        jmp     .next_part
@@:
        DEBUGF 1, "???\n"
        jmp     .next_part
.part_done:
        xor     eax, eax
        ret
.error:
        movi    eax, 1
        ret
endp


public kos_disk_del
proc kos_disk_del c uses ebx esi edi ebp, _name
        mov     eax, [disk_list+LHEAD.next]
.next_disk:
        cmp     eax, disk_list
        jz      .not_found
        mov     esi, [eax+DISK.Name]
        mov     edi, [_name]
@@:
        movzx   ecx, byte[esi]
        cmpsb
        jnz     .skip
        jecxz   .found
        jmp     @b
.skip:
        mov     eax, [eax+LHEAD.next]
        jmp     .next_disk
.found:
        stdcall disk_del, eax
        xor     eax, eax
        ret
.not_found:
        movi    eax, 1
        ret
endp


proc vdisk_close stdcall uses ebx esi edi ebp, _userdata
        extrn cio_disk_free
        ccall   cio_disk_free, [_userdata]
        ret
endp


proc vdisk_read stdcall uses ebx esi edi ebp, _userdata, _buffer, _startsector:qword, _numsectors
        extrn cio_disk_read
        ccall   cio_disk_read, [_userdata], [_buffer], dword[_startsector+0], dword[_startsector+4], [_numsectors]
        movi    eax, DISK_STATUS_OK
        ret
endp


proc vdisk_write stdcall uses ebx esi edi ebp, userdata, buffer, startsector:qword, numsectors
        extrn cio_disk_write
        ccall   cio_disk_write, [userdata], [buffer], dword[startsector+0], dword[startsector+4], [numsectors]
        movi    eax, DISK_STATUS_OK
        ret
endp


proc vdisk_querymedia stdcall uses ebx esi edi ebp, vdisk, mediainfo
        mov     ecx, [mediainfo]
        mov     edx, [vdisk]
        mov     [ecx + DISKMEDIAINFO.Flags], 0
        mov     eax, [edx + VDISK.SectSize]
        mov     [ecx + DISKMEDIAINFO.SectorSize], eax
        mov     eax, [edx + VDISK.SectCnt.lo]
        mov     dword [ecx + DISKMEDIAINFO.Capacity + 0], eax
        mov     eax, [edx + VDISK.SectCnt.hi]
        mov     dword [ecx + DISKMEDIAINFO.Capacity + 4], eax
        
        movi    eax, DISK_STATUS_OK
        ret
endp


proc __malloc
        push    ecx edx
        ccall   _malloc, eax
        pop     edx ecx
        ret
endp


proc kernel_alloc _size
        mov     eax, [_size]
        call    malloc
        ret
endp


proc alloc_kernel_space _size
        mov     eax, [_size]
        call    malloc
        ret
endp


proc alloc_pages _cnt
        mov     eax, [_cnt]
        shl     eax, 12
        call    malloc
        ret
endp


proc __free
        ccall   _free, eax
        ret
endp


proc kernel_free base
        mov     eax, [base]
        call    free
        xor     eax, eax
        ret
endp


proc _sys_msg_board
        cmp     cl, 0x0d
        jz      @f
        pushad
        mov     eax, SYS_WRITE
        mov     ebx, STDOUT
        push    ecx
        mov     ecx, esp
        mov     edx, 1
        int     0x80
        pop     ecx
        popad
@@:
        ret
endp


align 16        ;very often call this subrutine
memmove:       ; memory move in bytes
; eax = from
; ebx = to
; ecx = no of bytes
        test    ecx, ecx
        jle     .ret

        push    esi edi ecx

        mov     edi, ebx
        mov     esi, eax

        test    ecx, not 11b
        jz      @f

        push    ecx
        shr     ecx, 2
        rep movsd
        pop     ecx
        and     ecx, 11b
        jz      .finish
;--------------------------------------
align 4
@@:
        rep movsb
;--------------------------------------
align 4
.finish:
        pop     ecx edi esi
;--------------------------------------
align 4
.ret:
        ret

proc my_putpixel
        DEBUGF 1,"hello from my putpixel!\n"
        ret
endp

sys_msg_board_str:
protect_from_terminate:
unprotect_from_terminate:
change_task:
ReadCDWRetr:
WaitUnitReady:
prevent_medium_removal:
Read_TOC:
commit_pages:
release_pages:
fs_execute:
mutex_init:
mutex_lock:
mutex_unlock:
alloc_page:
map_page:
free_kernel_space:
cache_ide0:
cache_ide1:
wakeup_osloop:
read_process_memory:
.forced:
        ret
sys_getkey:
sys_clock:
delay_hs_unprotected:
undefined_syscall:
sys_cpuusage:
sys_waitforevent:
sys_getevent:
sys_redrawstat:
syscall_getscreensize:
sys_background:
sys_cachetodiskette:
sys_getbutton:
sys_system:
paleholder:
sys_midi:
sys_setup:
sys_settime:
sys_wait_event_timeout:
syscall_cdaudio:
syscall_putarea_backgr:
sys_getsetup:
sys_date:
syscall_getpixel_WinMap:
syscall_getarea:
readmousepos:
sys_getbackground:
set_app_param:
sys_outport:
syscall_reserveportarea:
sys_apm:
syscall_threads:
sys_clipboard:
sound_interface:
sys_pcibios:
sys_IPC:
sys_gs:
pci_api:
sys_resize_app_memory:
sys_process_def:
f68:
sys_debug_services:
sys_sendwindowmsg:
blit_32:
sys_network:
sys_socket:
sys_protocols:
sys_posix:
sys_end:
        ret

proc __load_file _filename
        push    ebx ecx edx esi edi
        stdcall kernel_alloc, skin_size
        push    eax
        mov     esi, skin
        mov     edi, eax
        mov     ecx, skin_size
        rep movsb
        pop     eax
        pop     edi esi edx ecx ebx
        ret
endp

coverage_end:
public coverage_end


section '.data' writeable align 64
include_debug_strings
vdisk_functions:
        dd vdisk_functions_end - vdisk_functions
        dd 0;vdisk_close    ; close
        dd 0    ; closemedia
        dd vdisk_querymedia
        dd vdisk_read
        dd vdisk_write
        dd 0    ; flush
        dd 0    ; adjust_cache_size
vdisk_functions_end:

disk_name db 'hd0',0
;IncludeIGlobals
skin file 'skin.skn'
skin_size = $ - skin
include 'hid/mousedrv.inc'
include 'gui/mousepointer.inc'

screen_workarea RECT
display_width_standard dd 0
display_height_standard dd 0
do_not_touch_winmap db 0
window_minimize db 0
sound_flag      db 0

fl_moving db 0
rb 3

;section '.bss' writeable align 16
;IncludeUGlobals
; crap
DiskNumber db ?
ChannelNumber db ?
DevErrorCode dd ?
CDSectorAddress dd ?
CDDataBuf_pointer dd ?
;DRIVE_DATA: rb 0x4000
;cdpos dd ?
;cd_appl_data dd ?
;current_slot dd ?
;pg_data PG_DATA
ide_channel1_mutex MUTEX
ide_channel2_mutex MUTEX
ide_channel3_mutex MUTEX
ide_channel4_mutex MUTEX
ide_channel5_mutex MUTEX
ide_channel6_mutex MUTEX
ide_channel7_mutex MUTEX
ide_channel8_mutex MUTEX
os_base rb 0x400000
public lfb_base_addr as 'kos_lfb_base'
lfb_base_addr dd lfb_base
lfb_base rd MAX_SCREEN_WIDTH*MAX_SCREEN_HEIGHT
cur_dir:
.encoding rb 1
.path     rb maxPathLength

BgrAuxTable     rb  32768
BgrAuxTable     equ
SB16Buffer      rb  65536
SB16Buffer      equ
BUTTON_INFO     rb  64*1024
BUTTON_INFO     equ

include 'data32.inc'

format ELF

__DEBUG__ = 1
__DEBUG_LEVEL__ = 1

extrn 'malloc' as libc_malloc
extrn 'free' as libc_free

cli equ nop
iretd equ retd

lang fix en

macro int n {
  if n eq 0x40
    call i40
  else
    int n
  end if
}

UEFI = 1

MAX_PRIORITY      = 0   ; highest, used for kernel tasks
USER_PRIORITY     = 1   ; default
IDLE_PRIORITY     = 2   ; lowest, only IDLE thread goes here

purge mov,add,sub
purge mov,add,sub
section '.text' executable align 32

public i40

coverage_begin:
public coverage_begin

include 'macros.inc'
macro diff16 msg,blah2,blah3 {
  if msg eq "end of .data segment"
    section '.bss' writeable align 64
  end if
}
include 'proc32.inc'
include 'struct.inc'
macro BOOT_LO a {}
macro BOOT a {}
window_data equ twer
CURRENT_TASK equ twer2
TASK_COUNT equ gyads
SLOT_BASE equ gfdskh
include 'const.inc'
restore window_data
restore CURRENT_TASK
restore TASK_COUNT
restore SLOT_BASE
purge BOOT_LO,BOOT

LFB_BASE = lfb_base

;window_data        = os_base + 0x00001000
;CURRENT_TASK       = os_base + 0x00003000
;TASK_COUNT         = os_base + 0x00003004
TASK_BASE          = os_base + 0x00003010
TASK_DATA          = os_base + 0x00003020
TASK_EVENT         = os_base + 0x00003020
CDDataBuf          = os_base + 0x00005000
idts               = os_base + 0x0000B100
WIN_STACK          = os_base + 0x0000C000
WIN_POS            = os_base + 0x0000C400
FDD_BUFF           = os_base + 0x0000D000
WIN_TEMP_XY        = os_base + 0x0000F300
KEY_COUNT          = os_base + 0x0000F400
KEY_BUFF           = os_base + 0x0000F401 ; 120*2 + 2*2 = 244 bytes, actually 255 bytes
BTN_COUNT          = os_base + 0x0000F500
BTN_BUFF           = os_base + 0x0000F501
BTN_ADDR           = os_base + 0x0000FE88
MEM_AMOUNT         = os_base + 0x0000FE8C
SYS_SHUTDOWN       = os_base + 0x0000FF00
TASK_ACTIVATE      = os_base + 0x0000FF01
sys_proc           = os_base + 0x0006F000
SLOT_BASE          = os_base + 0x00080000
VGABasePtr         = os_base + 0x000A0000
UPPER_KERNEL_PAGES = os_base + 0x00400000
HEAP_BASE          = os_base + 0x00800000

include 'system.inc'
include 'fdo.inc'
include 'blkdev/disk.inc'
include 'blkdev/disk_cache.inc'
include 'fs/fs_lfn.inc'
include 'crc.inc'
include 'core/string.inc'
include 'core/malloc.inc'
include 'core/heap.inc'
include 'core/dll.inc'
include 'core/taskman.inc'
include 'core/timers.inc'
include 'core/clipboard.inc'
include 'core/syscall.inc'
include 'video/framebuffer.inc'
include 'video/vesa20.inc'
include 'video/vga.inc'
include 'video/blitter.inc'
include 'video/cursors.inc'
include 'sound/playnote.inc'
include 'unpacker.inc'
include 'gui/window.inc'
include 'gui/button.inc'
include 'gui/skincode.inc'
include 'gui/font.inc'
include 'gui/event.inc'
include 'gui/mouse.inc'
include 'hid/keyboard.inc'
include 'hid/mousedrv.inc'
include 'network/stack.inc'
 
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
        mov     edi, endofcode
        mov     ecx, uglobals_size
        xor     eax, eax
        rep stosb

        MEMORY_BYTES = 128 SHL 20
        DISPLAY_WIDTH = 400
        DISPLAY_HEIGHT = 300
        mov     [pg_data.mem_amount], MEMORY_BYTES
        mov     [pg_data.pages_count], MEMORY_BYTES / PAGE_SIZE
        mov     [pg_data.pages_free], MEMORY_BYTES / PAGE_SIZE
        mov     eax, MEMORY_BYTES SHR 12
        mov     [pg_data.kernel_pages], eax
        shr     eax, 10
        mov     [pg_data.kernel_tables], eax
        call    init_kernel_heap
        call    init_malloc

        mov     [BOOT.bpp], 32
        mov     [BOOT.x_res], 400
        mov     [BOOT.y_res], 300
        mov     [BOOT.pitch], 400*4
        mov     [BOOT.lfb], LFB_BASE
        call    init_video

        stdcall kernel_alloc, (unpack.LZMA_BASE_SIZE+(unpack.LZMA_LIT_SIZE shl \
                (unpack.lc+unpack.lp)))*4
        mov     [unpack.p], eax

        call    init_events
        mov     eax, srv.fd-SRV.fd
        mov     [srv.fd], eax
        mov     [srv.bk], eax

        mov     dword[sysdir_name], 'sys'
        mov     dword[sysdir_path], 'RD/1'
        mov     word[sysdir_path+4], 0

        mov     dword[CURRENT_TASK], 2
        mov     dword[TASK_COUNT], 2
        mov     dword[TASK_BASE], CURRENT_TASK + 2*sizeof.TASKDATA
        mov     [current_slot], SLOT_BASE + 256*2

        ;call    ramdisk_init

        mov     ebx, SLOT_BASE + 2*256
        stdcall kernel_alloc, 0x2000
        mov     [ebx+APPDATA.process], eax
        mov     word[cur_dir.path], '/'
        mov     [ebx+APPDATA.cur_dir], cur_dir
        mov     [ebx+APPDATA.wnd_clientbox.left], 0
        mov     [ebx+APPDATA.wnd_clientbox.top], 0

        mov     [X_UNDER], 500
        mov     [Y_UNDER], 500
        mov     word[MOUSE_X], 40
        mov     word[MOUSE_Y], 30

        stdcall kernel_alloc, [_display.win_map_size]
        mov     [_display.win_map], eax

; set background
        movi    eax, 1
        mov     [BgrDrawMode], eax
        mov     [BgrDataWidth], eax
        mov     [BgrDataHeight], eax
        mov     [mem_BACKGROUND], 4
        mov     [img_background], static_background_data

; set clipboard
        xor     eax, eax
        mov     [clipboard_slots], eax
        mov     [clipboard_write_lock], eax
        stdcall kernel_alloc, 4096
        test    eax, eax
        jnz     @f

        dec     eax
@@:
        mov     [clipboard_main_list], eax


        call    set_window_defaults
        call    init_background
        call    calculatebackground
        call    init_display
        mov     eax, [def_cursor]
        mov     [SLOT_BASE+APPDATA.cursor+256], eax
        mov     [SLOT_BASE+APPDATA.cursor+256*2], eax

        ; from set_variables
        xor     eax, eax
        mov     [BTN_ADDR], dword BUTTON_INFO ; address of button list
        mov     byte [KEY_COUNT], al              ; keyboard buffer
        mov     byte [BTN_COUNT], al              ; button buffer

        ;call    load_default_skin
        ;call    stack_init

        ret
endp

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

proc alloc_page
        ret
        push    ecx edx
        mov     eax, 0x1000
        ccall   libc_malloc
        pop     edx ecx
        ret
endp

proc alloc_pages _cnt
        ret
        push    ecx edx
        mov     eax, [_cnt]
        shl     eax, 12
        ccall   libc_malloc
        pop     edx ecx
        ret
endp

proc sys_msg_board
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


proc disable_irq _irq
        ret
endp

proc enable_irq _irq
        ret
endp

proc map_page _one, _two, _three
        ret
endp

proc create_ring_buffer stdcall, size:dword, flags:dword
        ret
endp

proc map_memEx stdcall, lin_addr:dword,slot:dword,\
                        ofs:dword,buf_size:dword,req_access:dword
        ret
endp

change_task:
        mov     [REDRAW_BACKGROUND], 0
        ret

sysfn_saveramdisk:
sysfn_meminfo:
check_fdd_motor_status:
check_ATAPI_device_event:
check_fdd_motor_status_has_work?:
check_ATAPI_device_event_has_work?:
get_clock_ns:
request_terminate:
system_shutdown:
terminate:
LoadMedium:
clear_CD_cache:
allow_medium_removal:
EjectMedium:
save_image:
init_irqs:
PIC_init:
init_sys_v86:
PIT_init:
ramdisk_init:
APIC_init:
unmask_timer:
pci_enum:
load_pe_driver:
usb_init:
fdc_init:
attach_int_handler:
mtrr_validate:
protect_from_terminate:
unprotect_from_terminate:
ReadCDWRetr:
WaitUnitReady:
prevent_medium_removal:
Read_TOC:
commit_pages:
release_pages:
mutex_init:
mutex_lock:
mutex_unlock:
lock_application_table:
unlock_application_table:
get_pg_addr:
free_page:
scheduler_add_thread:
build_interrupt_table:
init_fpu:
init_mtrr:
map_io_mem:
create_trampoline_pgmap:
        ret
sys_settime:
sys_pcibios:
sys_IPC:
pci_api:
sys_resize_app_memory:
f68:
sys_posix:
        ret


macro format a,b,c,d {
macro pew x \{
\}
}

pewpew:

macro pew x, y {
  if y eq 1
    include `x
  end if
}

1FFFFFFFh fix 0
include fix pew
HEAP_BASE equ
macro org x {}
include 'init.inc', 1
sys_msg_board equ tyu
include 'kernel.asm', 1
purge sys_msg_board
restore org
purge org
purge HEAP_BASE
restore pew

coverage_end:
public coverage_end


section '.data' writeable align 64
;include_debug_strings
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

timer_ticks dd 0
fpu_owner dd ?

public win_stack_addr as 'kos_win_stack'
win_stack_addr dd WIN_STACK
public win_pos_addr as 'kos_win_pos'
win_pos_addr dd WIN_POS
public lfb_base_addr as 'kos_lfb_base'
lfb_base_addr dd lfb_base

uglobal
context_counter dd ?
LAPIC_BASE dd ?
irq_mode dd ?
cache_ide0  IDE_CACHE
cache_ide1  IDE_CACHE
DiskNumber db ?
ChannelNumber db ?
DevErrorCode dd ?
CDSectorAddress dd ?
CDDataBuf_pointer dd ?
allow_dma_access db ?
ide_mutex MUTEX
ide_channel1_mutex MUTEX
ide_channel2_mutex MUTEX
ide_channel3_mutex MUTEX
ide_channel4_mutex MUTEX
ide_channel5_mutex MUTEX
ide_channel6_mutex MUTEX
ide_channel7_mutex MUTEX
ide_channel8_mutex MUTEX
os_base:        rb 0x1000
window_data:    rb 0x2000
CURRENT_TASK:   rb 4
TASK_COUNT:     rb 12
                rb 0x1000000
;os_base rb 0x1000000
BOOT_LO boot_data
BOOT boot_data
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
endg

macro iii {
inclu#de 'data32.inc'
}
iii

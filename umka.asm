format ELF

__DEBUG__ = 1
__DEBUG_LEVEL__ = 1

public disk_add
public disk_del
public disk_list
public disk_media_changed

public xfs._.user_functions as 'xfs_user_functions'
public ext_user_functions
public fat_user_functions
public ntfs_user_functions

public i40

public coverage_begin
public coverage_end

public sha3_256_oneshot as 'hash_oneshot'
public kos_time_to_epoch
public kos_init

public win_stack_addr as 'kos_win_stack'
public win_pos_addr as 'kos_win_pos'
public lfb_base_addr as 'kos_lfb_base'

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

section '.text' executable align 32

coverage_begin:

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
include 'unicode.inc'
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

proc sha3_256_oneshot c uses ebx esi edi ebp, _ctx, _data, _len
        stdcall sha3_256.oneshot, [_ctx], [_data], [_len]
        ret
endp

proc kos_time_to_epoch c uses ebx esi edi ebp, _time
        mov     esi, [_time]
        call    fsCalculateTime
        add     eax, 978307200  ; epoch to 2001.01.01
        ret
endp

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
alloc_page:

sys_settime:
sys_pcibios:
sys_IPC:
pci_api:
sys_resize_app_memory:
f68:
sys_posix:
        ret

alloc_pages:
enable_irq:
disable_irq:
        ret     4
create_ring_buffer:
        ret     8
map_page:
        ret     12
map_memEx:
        ret     20


macro include_ x {
  inclu#de `x
}

include fix pew
macro pew x {}
macro org x {}
macro format [x] {}
HEAP_BASE equ
include_ 'init.inc'
sys_msg_board equ __pew

macro lea r, v {
  if v eq [(ecx-(CURRENT_TASK and 1FFFFFFFh)-TASKDATA.state)*8+SLOT_BASE]
        int3
  else if v eq [(edx-(CURRENT_TASK and 1FFFFFFFh))*8+SLOT_BASE]
        int3
  else
        lea     r, v
  end if
}

macro add r, v {
  if v eq CURRENT_TASK - (SLOT_BASE shr 3)
        int3
  else
        add     r, v
  end if
}

include_ 'kernel.asm'

purge lea,add,org,pew
restore lea,add,org,pew
purge sys_msg_board,HEAP_BASE

coverage_end:


section '.data' writeable align 64
timer_ticks dd 0
fpu_owner dd ?

win_stack_addr dd WIN_STACK
win_pos_addr dd WIN_POS
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

include_ 'data32.inc'

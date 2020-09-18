format ELF

__DEBUG__ = 1
__DEBUG_LEVEL__ = 1

UMKA_SHELL = 1
UMKA_FUSE  = 2
UMKA_OS    = 3

UMKA_MEMORY_BYTES = 128 SHL 20
UMKA_DISPLAY_WIDTH = 400
UMKA_DISPLAY_HEIGHT = 300

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

public monitor_thread
public CURRENT_TASK as 'kos_current_task'
public current_slot as 'kos_current_slot'
public TASK_COUNT as 'kos_task_count'
public TASK_BASE as 'kos_task_base'
public TASK_DATA as 'kos_task_data'
public SLOT_BASE as 'kos_slot_base'

public WIN_STACK as 'kos_win_stack'
public WIN_POS as 'kos_win_pos'
public lfb_base as 'kos_lfb_base'

public enable_acpi
public acpi.call_name
public acpi_ssdt_cnt as 'kos_acpi_ssdt_cnt'
public acpi_ssdt_base as 'kos_acpi_ssdt_base'
public acpi_ssdt_size as 'kos_acpi_ssdt_size'
public acpi_ctx
public acpi_usage as 'kos_acpi_usage'

public stack_init
public net_add_device

public draw_data
public img_background
public BgrDataWidth
public BgrDataHeight
public mem_BACKGROUND
public sys_background
public REDRAW_BACKGROUND
public background_defined

macro cli {
        pushfd
        btr     dword[esp], 21
        popfd
}

macro sti {
        pushfd
        bts     dword[esp], 21
        popfd
}

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

macro save_ring3_context {
        pushad
}

macro restore_ring3_context {
        popad
}

macro add r, v {
  if v eq CURRENT_TASK - (SLOT_BASE shr 3)
        push    r
        mov     r, SLOT_BASE
        shr     r, 3
        neg     r
        add     r, CURRENT_TASK
        add     r, [esp]
        add     esp, 4
  else
        add     r, v
  end if
}

include 'system.inc'
include 'fdo.inc'
include 'blkdev/disk.inc'
include 'blkdev/disk_cache.inc'
include 'fs/fs_lfn.inc'
include 'crc.inc'
include 'unicode.inc'
include 'core/sched.inc'
include 'core/irq.inc'
include 'core/sync.inc'
include 'core/apic.inc'
include 'acpi/acpi.inc'
include 'core/string.inc'
include 'core/malloc.inc'
include 'core/heap.inc'
include 'core/dll.inc'
include 'core/taskman.inc'
include 'core/timers.inc'
include 'core/clipboard.inc'
include 'core/syscall.inc'
include 'core/slab.inc'
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

; TODO: stdcall attribute in umka.h
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

        mov     [xsave_area_size], 0x1000

        mov     ecx, pg_data.mutex
        call    mutex_init

        mov     ecx, disk_list_mutex
        call    mutex_init

        mov     ecx, keyboard_list_mutex
        call    mutex_init

        mov     ecx, unpack_mutex
        call    mutex_init

        mov     ecx, application_table_mutex
        call    mutex_init

        mov     ecx, ide_mutex
        call    mutex_init
        mov     ecx, ide_channel1_mutex
        call    mutex_init
        mov     ecx, ide_channel2_mutex
        call    mutex_init
        mov     ecx, ide_channel3_mutex
        call    mutex_init
        mov     ecx, ide_channel4_mutex
        call    mutex_init
        mov     ecx, ide_channel5_mutex
        call    mutex_init
        mov     ecx, ide_channel6_mutex
        call    mutex_init

        mov     [pg_data.mem_amount], UMKA_MEMORY_BYTES
        mov     [pg_data.pages_count], UMKA_MEMORY_BYTES / PAGE_SIZE
        mov     [pg_data.pages_free], UMKA_MEMORY_BYTES / PAGE_SIZE
        mov     eax, UMKA_MEMORY_BYTES SHR 12
        mov     [pg_data.kernel_pages], eax
        shr     eax, 10
        mov     [pg_data.kernel_tables], eax
        call    init_kernel_heap
        call    init_malloc

        mov     eax, sys_proc
        list_init eax
        add     eax, PROC.thr_list
        list_init eax

        mov     [BOOT.bpp], 32
        mov     [BOOT.x_res], UMKA_DISPLAY_WIDTH
        mov     [BOOT.y_res], UMKA_DISPLAY_HEIGHT
        mov     [BOOT.pitch], UMKA_DISPLAY_WIDTH*4
        mov     [BOOT.lfb], LFB_BASE
        call    init_video

        stdcall kernel_alloc, (unpack.LZMA_BASE_SIZE+(unpack.LZMA_LIT_SIZE shl \
                (unpack.lc+unpack.lp)))*4
        mov     [unpack.p], eax

        call    init_events
        mov     eax, srv.fd-SRV.fd
        mov     [srv.fd], eax
        mov     [srv.bk], eax

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

        mov     dword[sysdir_name], 'sys'
        mov     dword[sysdir_path], 'RD/1'
        mov     word[sysdir_path+4], 0

        ;call    ramdisk_init

        mov     [X_UNDER], 500
        mov     [Y_UNDER], 500
        mov     word[MOUSE_X], 40
        mov     word[MOUSE_Y], 30

        mov     eax, -1
        mov     edi, thr_slot_map+4
        mov     [edi-4], dword 0xFFFFFFF8
        stosd
        stosd
        stosd
        stosd
        stosd
        stosd
        stosd

        mov     [current_process], sys_proc

        mov     dword[CURRENT_TASK], 0
        mov     dword[TASK_COUNT], 0

        mov     eax, [xsave_area_size]
        add     eax, RING0_STACK_SIZE
        stdcall kernel_alloc, eax
        mov     ebx, eax
        mov     edx, SLOT_BASE+256*1
        call    setup_os_slot
        mov     dword[edx], 'IDLE'
        sub     [edx+APPDATA.saved_esp], 4
        mov     eax, [edx+APPDATA.saved_esp]
        mov     dword[eax], idle
        mov     ecx, IDLE_PRIORITY
        call    sched_add_thread

        mov     eax, [xsave_area_size]
        add     eax, RING0_STACK_SIZE
        stdcall kernel_alloc, eax
        mov     ebx, eax
        mov     edx, SLOT_BASE+256*2
        call    setup_os_slot
        mov     dword[edx], 'OS'
        sub     [edx+APPDATA.saved_esp], 4
        mov     eax, [edx+APPDATA.saved_esp]
        mov     dword[eax], 0
        xor     ecx, ecx
        call    sched_add_thread

        mov     dword[CURRENT_TASK], 2
        mov     dword[TASK_COUNT], 2
        mov     dword[TASK_BASE], CURRENT_TASK + 2*sizeof.TASKDATA
        mov     [current_slot], SLOT_BASE+256*2
        mov     [CURRENT_TASK + 2*sizeof.TASKDATA + TASKDATA.pid], 2

        call    set_window_defaults
        call    init_background
        call    calculatebackground
        call    init_display
        mov     eax, [def_cursor]
        mov     [SLOT_BASE+APPDATA.cursor+256], eax
        mov     [SLOT_BASE+APPDATA.cursor+256*2], eax

        ; from set_variables
        xor     eax, eax
        mov     [BTN_ADDR], dword BUTTON_INFO   ; address of button list
        mov     byte [KEY_COUNT], al            ; keyboard buffer
        mov     byte [BTN_COUNT], al            ; button buffer

        mov     ebx, SLOT_BASE + 2*256
        mov     word[cur_dir.path], '/'
        mov     [ebx+APPDATA.cur_dir], cur_dir

        ;call    stack_init

        ret
endp

public skin_udata
proc idle uses ebx esi edi
.loop:
        mov     ecx, 10000000
@@:
        loop    @b
;        DEBUGF 1, "1 idle\n"
        jmp     .loop

        ret
endp

extrn raise
public umka_os
proc umka_os uses ebx esi edi
        call    kos_init

        call    stack_init

        mov     edx, SLOT_BASE+256*3
        xor     ecx, ecx
        call    sched_add_thread

        mov     edx, SLOT_BASE+256*4
        xor     ecx, ecx
        call    sched_add_thread

        mov     edx, SLOT_BASE+256*5
        xor     ecx, ecx
        call    sched_add_thread

        mov     dword[TASK_COUNT], 5

        stdcall umka_install_thread, [monitor_thread]

        ccall   raise, SIGPROF

        jmp     osloop

        ret
endp

extrn pci_read
proc pci_read_reg uses ebx esi edi
        mov     ecx, eax
        and     ecx, 3
        movi    edx, 1
        shl     edx, cl
        push    edx     ; len
        movzx   edx, bl
        push    edx     ; offset
        movzx   edx, bh
        and     edx, 7
        push    edx     ; fun
        movzx   edx, bh
        shr     edx, 3
        push    edx     ; dev
        movzx   edx, ah
        push    edx     ; bus
        call    pci_read
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

proc map_io_mem _base, _size, _flags
        mov     eax, [_base]
        ret
endp

extrn umka_sched_add_thread
sched_add_thread:
        stdcall umka_sched_add_thread, edx
        ret

public umka_install_thread
proc umka_install_thread _func
        ; fpu_state = sigsetjmp
        mov     eax, [xsave_area_size]
        add     eax, RING0_STACK_SIZE
        stdcall kernel_alloc, eax
        mov     ebx, eax
        mov     edx, [TASK_COUNT]
        inc     edx
        shl     edx, 8
        add     edx, SLOT_BASE
        call    setup_os_slot
        mov     dword [edx], 'USER'
        sub     [edx+APPDATA.saved_esp], 4
        mov     eax, [edx+APPDATA.saved_esp]
        mov     ecx, [_func]
        mov     dword[eax], ecx
        xor     ecx, ecx
        call    sched_add_thread
        inc     dword[TASK_COUNT]
        ret
endp

sysfn_saveramdisk:
sysfn_meminfo:
check_fdd_motor_status:
check_ATAPI_device_event:
check_fdd_motor_status_has_work?:
check_ATAPI_device_event_has_work?:
request_terminate:
system_shutdown:
terminate:
LoadMedium:
clear_CD_cache:
allow_medium_removal:
EjectMedium:
save_image:
init_sys_v86:
ramdisk_init:
pci_enum:
load_pe_driver:
usb_init:
fdc_init:
mtrr_validate:
protect_from_terminate:
unprotect_from_terminate:
ReadCDWRetr:
WaitUnitReady:
prevent_medium_removal:
Read_TOC:
commit_pages:
release_pages:
lock_application_table:
unlock_application_table:
get_pg_addr:
free_page:
build_interrupt_table:
init_fpu:
init_mtrr:
create_trampoline_pgmap:
alloc_page:
pci_write_reg:

sys_settime:
sys_pcibios:
sys_IPC:
pci_api:
sys_resize_app_memory:
f68:
sys_posix:
v86_irq:
        ret

alloc_pages:
        ret     4
create_ring_buffer:
        ret     8
map_page:
        ret     12
map_memEx:
        ret     20


HEAP_BASE equ
include 'init.inc'
sys_msg_board equ __pew

include fix pew
macro pew x {}
macro pew x {inclu#de `x}
macro org x {}
macro format [x] {}

macro lea r, v {
  if v eq [(ecx-(CURRENT_TASK and 1FFFFFFFh)-TASKDATA.state)*8+SLOT_BASE]
        int3
  else if v eq [(edx-(CURRENT_TASK and 1FFFFFFFh))*8+SLOT_BASE]
        int3
  else
        lea     r, v
  end if
}

include 'kernel.asm'

purge lea,add,org
restore lea,add,org
purge sys_msg_board,HEAP_BASE

coverage_end:


section '.data' writeable align 64
public umka_tool
umka_tool dd ?
fpu_owner dd ?

monitor_thread dd ?

uglobal
v86_irqhooks rd IRQ_RESERVED*2
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
align 64
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

include 'data32.inc'

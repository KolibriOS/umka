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
public umka_init

public CURRENT_TASK as 'kos_current_task'
public current_slot as 'kos_current_slot'
public TASK_COUNT as 'kos_task_count'
public TASK_BASE as 'kos_task_base'
public TASK_DATA as 'kos_task_data'
public SLOT_BASE as 'kos_slot_base'

public WIN_STACK as 'kos_win_stack'
public WIN_POS as 'kos_win_pos'
public lfb_base as 'kos_lfb_base'

public RAMDISK as 'kos_ramdisk'
public ramdisk_init as 'kos_ramdisk_init'

public enable_acpi
public acpi.call_name
public acpi_ssdt_cnt as 'kos_acpi_ssdt_cnt'
public acpi_ssdt_base as 'kos_acpi_ssdt_base'
public acpi_ssdt_size as 'kos_acpi_ssdt_size'
public acpi_ctx
public acpi_usage as 'kos_acpi_usage'

public stack_init as 'kos_stack_init'
public net_add_device

public draw_data
public img_background
public mem_BACKGROUND
public sys_background
public REDRAW_BACKGROUND
public new_sys_threads as 'kos_new_sys_threads'
public osloop as 'kos_osloop'
public set_mouse_data as 'kos_set_mouse_data'
public scheduler_current as 'kos_scheduler_current'
public eth_input as 'kos_eth_input'
public net_buff_alloc as 'kos_net_buff_alloc'

macro cli {
        pushfd
        bts     dword[esp], 21
        popfd
}

macro sti {
        pushfd
        btr     dword[esp], 21
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
TASK_BASE equ twer3
TASK_DATA equ twer4
TASK_EVENT equ twer5
CDDataBuf equ twer6
idts equ twer7
WIN_STACK equ twer8
WIN_POS equ twer9
TASK_COUNT equ gyads
SLOT_BASE equ gfdskh
;macro OS_BASE [x] {
;  OS_BASE equ os_base
;}
include 'const.inc'
restore window_data
restore CURRENT_TASK
restore TASK_BASE,TASK_DATA,TASK_EVENT,CDDataBuf,idts,WIN_STACK,WIN_POS
restore TASK_COUNT
restore SLOT_BASE
purge BOOT_LO,BOOT

LFB_BASE = lfb_base

;window_data        = os_base + 0x00001000
;CURRENT_TASK       = os_base + 0x00003000
;TASK_COUNT         = os_base + 0x00003004
;TASK_BASE          = os_base + 0x00003010
;TASK_DATA          = os_base + 0x00003020
;TASK_EVENT         = os_base + 0x00003020
;CDDataBuf          = os_base + 0x00005000
;idts               = os_base + 0x0000B100
;WIN_STACK          = os_base + 0x0000C000
;WIN_POS            = os_base + 0x0000C400
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
        add     esp, 4
        add     r, [esp-4]
  else
        add     r, v
  end if
}

include 'system.inc'
include 'fdo.inc'

include 'core/sync.inc'
;include 'core/sys32.inc'
macro call target {
  if target eq do_change_task
        call    _do_change_task
  else
        call    target
  end if
}
do_change_task equ hjk
irq0 equ jhg
macro mov r, v {
  if r eq [CURRENT_TASK] & v eq bh
        push    ebx
        sub     ebx, SLOT_BASE
        mov     [CURRENT_TASK], bh
        pop     ebx
  else
        mov     r, v
  end if
}
include 'core/sched.inc'
purge mov
purge call
restore irq0
include 'core/syscall.inc'
;include 'core/fpu.inc'
;include 'core/memory.inc'
;include 'core/mtrr.inc'
include 'core/heap.inc'
include 'core/malloc.inc'
include 'core/taskman.inc'
include 'core/dll.inc'
;include 'core/peload.inc'
;include 'core/exports.inc'
include 'core/string.inc'
;include 'core/v86.inc'
include 'core/irq.inc'
include 'core/apic.inc'
include 'core/timers.inc'
include 'core/clipboard.inc'
include 'core/slab.inc'

include 'posix/posix.inc'

;include 'boot/shutdown.inc'

include 'video/vesa20.inc'
include 'video/blitter.inc'
include 'video/vga.inc'
include 'video/cursors.inc'
include 'video/framebuffer.inc'

include 'gui/window.inc'
include 'gui/event.inc'
include 'gui/font.inc'
include 'gui/button.inc'
include 'gui/mouse.inc'
include 'gui/skincode.inc'

include 'hid/keyboard.inc'
include 'hid/mousedrv.inc'
;include 'hid/set_dtc.inc'      ; setting date,time,clock and alarm-clock

include 'sound/playnote.inc'

;include 'bus/pci/pci32.inc'
;include 'bus/usb/init.inc'

;include 'blkdev/flp_drv.inc'   ; floppy driver
;include 'blkdev/fdc.inc'
;include 'blkdev/cd_drv.inc'    ; CD driver
;include 'blkdev/ide_cache.inc' ; CD cache
;include 'blkdev/hd_drv.inc'    ; HDD driver
;include 'blkdev/bd_drv.inc'    ; BIOS disks driver
include 'blkdev/rd.inc'         ; ramdisk driver
include 'blkdev/disk.inc'
include 'blkdev/disk_cache.inc'

include 'fs/fs_lfn.inc'

include 'network/stack.inc'

include 'crc.inc'
include 'unicode.inc'
include 'acpi/acpi.inc'

include 'unpacker.inc'

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

proc umka_init c uses ebx esi edi ebp
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

        stdcall alloc_kernel_space, 0x50000         ; FIXME check size
        mov     [default_io_map], eax

        add     eax, 0x2000
        mov     [ipc_tmp], eax
        mov     ebx, 0x1000

        add     eax, 0x40000
        mov     [proc_mem_map], eax

        add     eax, 0x8000
        mov     [proc_mem_pdir], eax

        add     eax, ebx
        mov     [proc_mem_tab], eax

        add     eax, ebx
        mov     [tmp_task_ptab], eax

        add     eax, ebx
        mov     [ipc_pdir], eax

        add     eax, ebx
        mov     [ipc_ptab], eax

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
        call    scheduler_add_thread

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
        call    scheduler_add_thread

        mov     dword[CURRENT_TASK], 2
        mov     dword[TASK_COUNT], 2
        mov     dword[TASK_BASE], CURRENT_TASK + 2*sizeof.TASKDATA
        mov     [current_slot], SLOT_BASE+2*sizeof.APPDATA
        mov     [CURRENT_TASK + 2*sizeof.TASKDATA + TASKDATA.pid], 2

        call    set_window_defaults
        call    init_background
        call    calculatebackground
        call    init_display
        mov     eax, [def_cursor]
        mov     [SLOT_BASE+APPDATA.cursor+sizeof.APPDATA], eax
        mov     [SLOT_BASE+APPDATA.cursor+sizeof.APPDATA*2], eax

        ; from set_variables
        xor     eax, eax
        mov     [BTN_ADDR], dword BUTTON_INFO   ; address of button list
        mov     byte [KEY_COUNT], al            ; keyboard buffer
        mov     byte [BTN_COUNT], al            ; button buffer

        mov     ebx, SLOT_BASE + 2*sizeof.APPDATA
        mov     word[cur_dir.path], '/'
        mov     [ebx+APPDATA.cur_dir], cur_dir

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

proc delay_ms

        ret
endp

public umka_cli
proc umka_cli
        cli     ; macro
        ret
endp

public umka_sti
proc umka_sti
        sti     ; macro
        ret
endp

extrn reset_procmask
extrn get_fake_if
public irq0
proc irq0 c, _signo, _info, _context
        DEBUGF 2, "### irq0\n"
        pushfd
        cli
        pushad

        inc     [timer_ticks]
        call    updatecputimes
        ccall   reset_procmask
        ccall   get_fake_if, [_context]
        test    eax, eax
        jnz     @f
        DEBUGF 2, "### cli\n"
        jmp     .done
@@:

        mov     bl, SCHEDULE_ANY_PRIORITY
        call    find_next_task
        jz      .done  ; if there is only one running process
        call    _do_change_task
.done:
        popad
        popfd
        ret
endp

proc _do_change_task
        mov     eax, [current_slot]
        sub     eax, SLOT_BASE
        shr     eax, 8
        mov     ecx, ebx
        sub     ecx, SLOT_BASE
        shr     ecx, 8
        DEBUGF 2, "### switching task from %d to %d\n",eax,ecx

        mov     esi, ebx
        xchg    esi, [current_slot]
; set new stack after saving old
        mov     [esi+APPDATA.saved_esp], esp
        mov     esp, [ebx+APPDATA.saved_esp]
        ret
endp

proc map_io_mem _base, _size, _flags
        mov     eax, [_base]
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
delay_ms equ __pew8

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

macro mov r, v {
  if v eq (HEAP_BASE-OS_BASE-CLEAN_ZONE) / 4
        int3
  else
        mov     r, v
  end if
}

include 'kernel.asm'

purge lea,add,org,mov
restore lea,add,org,mov
purge sys_msg_board,HEAP_BASE,__pew8

coverage_end:


section '.data' writeable align 4096
public umka_tool
umka_tool dd ?
fpu_owner dd ?

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
TASK_BASE       rd 4
TASK_DATA       rd 0x7f8
TASK_EVENT = TASK_DATA
CDDataBuf:      rd 0x1840
idts            rd 0x3c0
WIN_STACK       rw 0x200
WIN_POS         rw 0x600
                rb 0x1000000
BOOT_LO boot_data
BOOT boot_data
align 4096
lfb_base rd MAX_SCREEN_WIDTH*MAX_SCREEN_HEIGHT
align 4096
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

macro org x {
  if x eq (OS_BASE+0x0100000)
  else
    org x
  end if
}

macro align x {
  if x eq 65536
  else if x eq 4096
  else
    align x
  end if
}

macro assert [x] {}
include 'data32.inc'

; TODO: SPDX

if defined WIN32
    format MS COFF
else
    format ELF
end if

; win32:
;   pubsym name                    -> public name as "_name"
;   pubsym name, 20                -> public name as "_name@20"
;   pubsym name, no_mangle         -> public name
;   pubsym name, "name"            -> public name as "_name"
;   pubsym name, "name", 20        -> public name as "_name@20"
;   pubsym name, "name", no_mangle -> public name as "name"
; linux:
;   pubsym name                    -> public name
;   pubsym name, 20                -> public name
;   pubsym name, no_mangle         -> public name
;   pubsym name, "name"            -> public name as "name"
;   pubsym name, "name", 20        -> public name as "name"
;   pubsym name, "name", no_mangle -> public name as "name"
macro pubsym name, marg1, marg2 {
    if defined WIN32
        if marg1 eq no_mangle
            public name
        else if marg1 eqtype 20
            public name as '_' # `name # '@' # `marg1
        else if marg1 eqtype 'string'
            if marg2 eq no_mangle
                public name as marg1
            else if marg2 eqtype 20
                public name as '_' # marg1 # '@' # `marg2
            else
                public name as '_' # marg1
            end if
        else
            public name as '_' # `name
        end if
    else
        if marg1 eqtype 'string'
            public name as marg1
        else
            public name
        end if
    end if
}

; win32:
;   extrn name     -> extrn _name
;   extrn name, 20 -> extrn _name@20
; linux:
;   extrn name     -> extrn name
;   extrn name, 20 -> extrn name
macro extrn name, [argsize] {
    if defined WIN32
        if argsize eqtype 20
            extrn '_' # `name # '@' # `argsize as name
        else
            extrn '_' # `name as name
        end if
    else
        extrn name
    end if
}

__DEBUG__ = 1
__DEBUG_LEVEL__ = 1

UMKA_SHELL = 1
UMKA_FUSE  = 2
UMKA_OS    = 3

UMKA_MEMORY_BYTES = 256 SHL 20

pubsym disk_add, 16
pubsym disk_del, 4
pubsym disk_list
pubsym disk_media_changed, 8

pubsym xfs._.user_functions, 'xfs_user_functions'
pubsym ext_user_functions
pubsym fat_user_functions
pubsym ntfs_user_functions

pubsym i40, no_mangle

pubsym coverage_begin
pubsym coverage_end

pubsym sha3_256_oneshot, 'hash_oneshot'
pubsym kos_time_to_epoch
pubsym umka_init

pubsym current_process, 'kos_current_process'
pubsym current_slot, 'kos_current_slot'
pubsym current_slot_idx, 'kos_current_slot_idx'

pubsym thread_count, 'kos_thread_count'
pubsym TASK_TABLE, 'kos_task_table'
pubsym TASK_BASE, 'kos_task_base'
pubsym TASK_DATA, 'kos_task_data'
pubsym SLOT_BASE, 'kos_slot_base'
pubsym window_data, 'kos_window_data'

pubsym WIN_STACK, 'kos_win_stack'
pubsym WIN_POS, 'kos_win_pos'
pubsym lfb_base, 'kos_lfb_base'

pubsym RAMDISK, 'kos_ramdisk'
pubsym ramdisk_init, 'kos_ramdisk_init'

pubsym enable_acpi, no_mangle
pubsym acpi.call_name, no_mangle
pubsym acpi_ssdt_cnt, 'kos_acpi_ssdt_cnt'
pubsym acpi_ssdt_base, 'kos_acpi_ssdt_base'
pubsym acpi_ssdt_size, 'kos_acpi_ssdt_size'
pubsym acpi_ctx
pubsym acpi_usage, 'kos_acpi_usage'
pubsym acpi_node_alloc_cnt, 'kos_acpi_node_alloc_cnt'
pubsym acpi_node_free_cnt, 'kos_acpi_node_free_cnt'
pubsym acpi.count_nodes, 'kos_acpi_count_nodes', 4

pubsym stack_init, 'kos_stack_init'
pubsym net_add_device

pubsym draw_data
pubsym img_background
pubsym mem_BACKGROUND
pubsym sys_background
pubsym REDRAW_BACKGROUND, 'kos_redraw_background'
pubsym new_sys_threads, 'kos_new_sys_threads', no_mangle
pubsym osloop, 'kos_osloop'
pubsym set_mouse_data, 'kos_set_mouse_data', 20
pubsym scheduler_current, 'kos_scheduler_current'
pubsym kos_eth_input
pubsym net_buff_alloc, 'kos_net_buff_alloc', 4

pubsym mem_block_list
pubsym pci_root, "kos_pci_root"

pubsym acpi.aml.init, "kos_acpi_aml_init"
pubsym acpi_root, "kos_acpi_root"
pubsym aml._.attach, "kos_aml_attach"
pubsym acpi.fill_pci_irqs, "kos_acpi_fill_pci_irqs"
pubsym pci.walk_tree, "kos_pci_walk_tree", 16
pubsym acpi.aml.new_thread, "kos_acpi_aml_new_thread"
pubsym aml._.alloc_node, "kos_aml_alloc_node"
pubsym aml._.constructor.integer, "kos_aml_constructor_integer"
pubsym aml._.constructor.package, "kos_aml_constructor_package"
pubsym acpi._.lookup_node, "kos_acpi_lookup_node"
pubsym acpi._.print_tree, "kos_acpi_print_tree"
pubsym acpi_dev_data, "kos_acpi_dev_data"
pubsym acpi_dev_size, "kos_acpi_dev_size"
pubsym acpi_dev_next, "kos_acpi_dev_next"
pubsym kernel_alloc, "kos_kernel_alloc"

pubsym window._.set_screen, 'kos_window_set_screen'
pubsym _display, 'kos_display'

pubsym BOOT, 'kos_boot'

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

section '.text' executable align 64

coverage_begin:

include 'macros.inc'

macro diff16 msg,blah2,blah3 {
  if msg eq "end of .data segment"
    if defined WIN32
      section '.bss.8k' writeable align 8192
    else
      ; fasm doesn't align on 65536, but ld script does
      section '.bss.aligned65k' writeable align 65536
    end if
bss_base:
  end if
}
include 'proc32.inc'
include 'struct.inc'
macro BOOT_LO a {}
macro BOOT a {}
window_data equ __pew01
TASK_TABLE equ __pew02
TASK_BASE equ __pew03
TASK_DATA equ __pew04
CDDataBuf equ __pew06
idts equ __pew07
WIN_STACK equ __pew08
WIN_POS equ __pew09
FDD_BUFF equ __pew10
WIN_TEMP_XY equ __pew11
KEY_COUNT equ __pew12
KEY_BUFF equ __pew13
BTN_COUNT equ __pew14
BTN_BUFF equ __pew15
BTN_ADDR equ __pew16
MEM_AMOUNT equ __pew17
SYS_SHUTDOWN equ __pew18
SLOT_BASE equ __pew20
sys_proc equ __pew21
VGABasePtr equ __pew22
HEAP_BASE equ __pew23
;macro OS_BASE [x] {
;  OS_BASE equ os_base
;}
include 'const.inc'
restore window_data
restore TASK_TABLE
restore TASK_BASE,TASK_DATA,CDDataBuf,idts,WIN_STACK,WIN_POS
restore FDD_BUFF,WIN_TEMP_XY,KEY_COUNT,KEY_BUFF,BTN_COUNT,BTN_BUFF,BTN_ADDR
restore MEM_AMOUNT,SYS_SHUTDOWN,SLOT_BASE,sys_proc,VGABasePtr
restore HEAP_BASE
purge BOOT_LO,BOOT

LFB_BASE = lfb_base

macro save_ring3_context {
        pushad
}

macro restore_ring3_context {
        popad
}

macro add r, v {
  if v eq TASK_TABLE - (SLOT_BASE shr 3)
        push    r
        mov     r, SLOT_BASE
        shr     r, 3
        neg     r
        add     r, TASK_TABLE
        add     esp, 4
        add     r, [esp-4]
  else
        add     r, v
  end if
}

macro stdcall target, [args] {
common
  if target eq is_region_userspace
        cmp     esp, esp        ; ZF
  else
        stdcall target, args
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
;macro mov r, v {
;  if r eq byte [current_slot_idx] & v eq bh
;        push    eax
;        mov     eax, ebx
;        sub     eax, SLOT_BASE
;        shr     eax, BSF sizeof.APPDATA
;        mov     [current_slot_idx], eax
;        pop     eax
;  else
;        mov     r, v
;  end if
;}
do_change_task equ hjk
irq0 equ jhg
include 'core/sched.inc'
purge call, mov
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
include 'core/hpet.inc'
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

LIBCRASH_CTX_LEN = 0x500   ; FIXME
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

proc umka._.check_alignment
        mov     eax, HEAP_BASE
        and     eax, PAGE_SIZE - 1
        jz      @f
        mov     ecx, PAGE_SIZE
        sub     ecx, eax
        DEBUGF 4, "HEAP_BASE must be aligned on PAGE_SIZE: 0x%x", HEAP_BASE
        DEBUGF 4, ", add 0x%x or sub 0x%x\n", ecx, eax
        int3
@@:
        ret
endp

pubsym i40_asm

;void i40_asm(uint32_t _eax,
;             uint32_t _ebx,
;             uint32_t _ecx,
;             uint32_t _edx,
;             uint32_t _esi,
;             uint32_t _edi,
;             uint32_t _ebp,
;             uint32_t *_eax_out,
;             uint32_t _ebx_out)
i40_asm:
        ; Return address: esp
        ; First argument: esp + 4
        push    eax ebx ecx edx esi edi ebp
        ; First argument: esp + 4 + 7 * sizeof(dword) = esp + 8 + 7 * 4 = esp + 4 + 28 = esp + 32
        mov     eax, [esp + 32]
        mov     ebx, [esp + 36]
        mov     ecx, [esp + 40]
        mov     edx, [esp + 44]
        mov     esi, [esp + 48]
        mov     edi, [esp + 52]
        mov     ebp, [esp + 56]
        call    i40
        mov     edi, [esp + 60]
        mov     [edi], eax
        mov     edi, [esp + 64]
        mov     [edi], ebx
        pop     ebp edi esi edx ecx ebx eax
        ret

pubsym set_eflags_tf

proc set_eflags_tf c uses ebx esi edi ebp, tf
        mov    ecx, [tf]
        pushfd
        pop    eax
        ror    eax, 8
        mov    edx, eax
        and    edx, 1
        and    eax, 0xfffffffe
        or     eax, ecx
        rol    eax, 8
        push   eax
        popfd
        mov    eax, edx
        ret
endp

proc kos_eth_input c uses ebx esi edi ebp, buffer_ptr
        push    .retaddr
        push    [buffer_ptr]
        jmp     eth_input
.retaddr:
        ret
endp

proc umka_init c uses ebx esi edi ebp
        mov     [umka_initialized], 1
        call    umka._.check_alignment

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

        mov     [current_slot_idx], 0
        mov     [thread_count], 0

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

        mov     [current_slot_idx], 2
        mov     [thread_count], 2
        mov     dword[TASK_BASE], TASK_TABLE + 2*sizeof.TASKDATA
        mov     [current_slot], SLOT_BASE+2*sizeof.APPDATA
        mov     [TASK_TABLE + 2*sizeof.TASKDATA + TASKDATA.pid], 2

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

pubsym skin_udata
proc idle uses ebx esi edi
.loop:
        mov     ecx, 10000000
@@:
        loop    @b
;        DEBUGF 1, "1 idle\n"
        jmp     .loop

        ret
endp

extrn pci_read, 20
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
if defined WIN32
        extrn   putchar
        pushad
        push    ecx
        call    putchar
        pop     ecx
        popad
else
        pushad
        mov     eax, SYS_WRITE
        mov     ebx, STDOUT
        push    ecx
        mov     ecx, esp
        mov     edx, 1
        int     0x80
        pop     ecx
        popad
end if
@@:
        ret
endp

proc delay_ms

        ret
endp

pubsym umka_cli
proc umka_cli
        cli     ; macro
        ret
endp

pubsym umka_sti
proc umka_sti
        sti     ; macro
        ret
endp

extrn reset_procmask
extrn get_fake_if
pubsym irq0
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
test_cpu:
acpi_locate:
init_BIOS32:
mem_test:
init_mem:
init_page_map:
ahci_init:
enable_acpi:
acpi.call_name:
acpi.count_nodes:
acpi.aml.init:
aml._.attach:
acpi.fill_pci_irqs:
pci.walk_tree:
acpi.aml.new_thread:
aml._.alloc_node:
aml._.constructor.integer:
aml._.constructor.package:
acpi._.lookup_node:
acpi._.print_tree:
        ret

alloc_pages:
        ret     4
create_ring_buffer:
        ret     8
map_page:
        ret     12
map_memEx:
        ret     20

uglobal
acpi_ctx dd ?
acpi_usage dd ?
acpi_node_alloc_cnt dd ?
acpi_node_free_cnt dd ?
pci_root dd ?
acpi_root dd ?
acpi_dev_next dd ?
endg

sys_msg_board equ __pex0
delay_ms equ __pex1

include fix pew
macro pew x {}
macro pew x {inclu#de `x}
macro org x {}
macro format [x] {}

macro lea r, v {
  if v eq [(ecx-(TASK_TABLE and 1FFFFFFFh)-TASKDATA.state)*8+SLOT_BASE]
        int3
  else if v eq [(edx-(TASK_TABLE and 1FFFFFFFh))*8+SLOT_BASE]
        int3
  else
        lea     r, v
  end if
}

include 'kernel.asm'

purge lea,add,org,mov
restore lea,add,org,mov
purge sys_msg_board,delay_ms
restore sys_msg_board,delay_ms

coverage_end:

if defined WIN32
    section '.data.8k' writeable align 8192
else
    ; fasm doesn't align on 65536, but ld script does
    section '.data.aligned65k' writeable align 65536
end if
pubsym umka_tool
umka_tool dd ?
pubsym umka_initialized
umka_initialized dd 0
fpu_owner dd ?

BOOT boot_data
virtual at BOOT
BOOT_LO boot_data
end virtual

uglobal
align 64
os_base:        rb PAGE_SIZE
window_data:    rb sizeof.WDATA * 256
TASK_TABLE:     rb 16
TASK_BASE:      rd 4
TASK_DATA:      rd sizeof.TASKDATA * 255 / 4
CDDataBuf:      rb 0x1000
idts            rb IRQ_RESERVED * 8     ; IDT descriptor is 8 bytes long
WIN_STACK       rw 0x200        ; why not 0x100?
WIN_POS         rw 0x200
FDD_BUFF:       rb 0x400
WIN_TEMP_XY     rb 0x100
KEY_COUNT       db ?
KEY_BUFF        rb 255  ; 120*2 + 2*2 = 244 bytes, actually 255 bytes
BTN_COUNT       db ?
BTN_BUFF        rd 0x261
BTN_ADDR        dd ?
MEM_AMOUNT      rd 0x1d
SYS_SHUTDOWN    db ?
sys_proc:       rb sizeof.PROC * 256
                rb 0x10000 - (($-bss_base) AND (0x10000-1)) ; align on 0x10000
SLOT_BASE:      rb sizeof.APPDATA * 256
VGABasePtr      rb 640*480
                rb PAGE_SIZE - (($-bss_base) AND (PAGE_SIZE-1)) ; align on page
HEAP_BASE       rb UMKA_MEMORY_BYTES - (HEAP_BASE - os_base + \
                                        PAGE_SIZE * sizeof.MEM_BLOCK)
endg

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

lfb_base        rd MAX_SCREEN_WIDTH*MAX_SCREEN_HEIGHT

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

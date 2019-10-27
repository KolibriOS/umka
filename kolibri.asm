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

purge mov,add,sub
purge mov,add,sub
section '.text' executable align 16

coverage_begin:
public coverage_begin

include 'macros.inc'
include 'proc32.inc'
include 'struct.inc'
include 'const.inc'
include 'system.inc'
include 'fdo.inc'
include 'blkdev/disk.inc'
include 'blkdev/disk_cache.inc'
include 'fs/fs_lfn.inc'
include 'crc.inc'

include 'sha3.asm'

SLOT_BASE    = os_base + 0x00080000

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
proc kos_init c
        MEMORY_BYTES = 128 SHL 20
        mov     [pg_data.mem_amount], MEMORY_BYTES
        mov     [pg_data.pages_count], MEMORY_BYTES / PAGE_SIZE
        mov     [pg_data.pages_free], MEMORY_BYTES / PAGE_SIZE

        mov     dword[sysdir_name], 'sys'
        mov     dword[sysdir_path], 'HD0/'
        mov     word[sysdir_path+4], '1'

        mov     eax, SLOT_BASE + 2*256
        mov     dword[current_slot], eax
        mov     word[cur_dir.path], '/'
        mov     [eax+APPDATA.cur_dir], cur_dir
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


public kos_lfn
proc kos_lfn c uses ebx edx esi edi ebp, _f70arg, _f70ret
        push    ebx
        mov     ebx, [_f70arg]
        pushad  ; file_system_lfn writes here
        call    file_system_lfn
        popad
        mov     ecx, [_f70ret]
        mov     [ecx+0], eax    ; status
        mov     [ecx+4], ebx    ; count
        pop     ebx
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
        ret

coverage_end:
public coverage_end


section '.data' writeable align 16
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
IncludeIGlobals


section '.bss' writeable align 16
IncludeUGlobals
; crap
DiskNumber db ?
ChannelNumber db ?
DevErrorCode dd ?
CDSectorAddress dd ?
CDDataBuf_pointer dd ?
DRIVE_DATA: rb 0x4000
cdpos dd ?
cd_appl_data dd ?
current_slot dd ?
pg_data PG_DATA
ide_channel1_mutex MUTEX
ide_channel2_mutex MUTEX
ide_channel3_mutex MUTEX
ide_channel4_mutex MUTEX
ide_channel5_mutex MUTEX
ide_channel6_mutex MUTEX
ide_channel7_mutex MUTEX
ide_channel8_mutex MUTEX
os_base rb 0x100000
cur_dir:
.encoding rb 1
.path     rb maxPathLength

format ELF

__DEBUG__ = 1
__DEBUG_LEVEL__ = 1

extrn malloc
malloc fix __malloc
_malloc fix malloc
extrn free
free fix __free
_free fix free

macro coverage_magic_instruction {
        nop
        int3
}

purge section,mov,add,sub
purge section,mov,add,sub
section '.text' executable align 16

coverage_begin:
public coverage_begin

include 'macros.inc'
include 'proc32.inc'
include 'struct.inc'
include 'const.inc'
include 'system.inc'
include 'debug-fdo.inc'
include 'blkdev/disk.inc'
include 'blkdev/disk_cache.inc'
include 'fs/fs_lfn.inc'
include 'crc.inc'

struct FILE_DISK
  fd      dd ?
  Sectors dd ?
  Logical dd ?  ; sector size
ends

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

;uint32_t kos_time_to_epoch(uint8_t *time);
public kos_time_to_epoch
kos_time_to_epoch:
        push    ebx esi edi ebp

        mov     esi, [esp + 0x14]
        call    fsCalculateTime
        add     eax, 978307200  ; epoch to 2001.01.01

        pop     ebp edi esi ebx
        ret


;void *kos_fuse_init(int fd, uint32_t sect_cnt, uint32_t sect_sz);
public kos_fuse_init
kos_fuse_init:
        push    ebx esi edi ebp

;DEBUGF 1, "coverage begin: 0x%x\n", coverage_begin
;DEBUGF 1, "xfs_create_partition: 0x%x\n", xfs_create_partition
;DEBUGF 1, "coverage end: 0x%x\n", coverage_end

        MEMORY_BYTES = 128 SHL 20
        mov     [pg_data.mem_amount], MEMORY_BYTES
        mov     [pg_data.pages_count], MEMORY_BYTES / PAGE_SIZE
        mov     [pg_data.pages_free], MEMORY_BYTES / PAGE_SIZE

        mov     eax, [esp + 0x14]
        mov     [file_disk.fd], eax
        mov     eax, [esp + 0x18]
        mov     [file_disk.Sectors], eax
        mov     eax, [esp + 0x1c]
        mov     [file_disk.Logical], eax
        stdcall disk_add, disk_functions, disk_name, file_disk, DISK_NO_INSERT_NOTIFICATION
        push    eax
        stdcall disk_media_changed, eax, 1
        pop     eax
        mov     eax, [eax + DISK.NumPartitions]

        pop     ebp edi esi ebx
        ret


public kos_fuse_lfn
;proc kos_fuse_lfn c, _blah1, _blah2
kos_fuse_lfn:
        push    ebx
        mov     ebx, [esp + 8]
        pushad  ; file_system_lfn writes here
        call    file_system_lfn
        popad
        mov     ecx, [esp + 12]
        mov     [ecx + 0], eax
        mov     [ecx + 4], ebx
        pop     ebx
        ret
;endp


proc disk_read stdcall, userdata, buffer, startsector:qword, numsectors
        pushad
        mov     eax, dword[startsector + 0]   ; sector lo
        mov     edx, dword[startsector + 4]   ; sector hi
        mov     edx, [userdata]
        mul     [edx + FILE_DISK.Logical]
        mov     ecx, edx
        mov     edx, eax
;DEBUGF 1, "lseek to: %x\n", edx
        mov     eax, [userdata]
        mov     ebx, [eax + FILE_DISK.fd]
        sub     esp, 8
        mov     esi, esp
        mov     edi, SEEK_SET
        mov     eax, SYS_LLSEEK
        int     0x80
        add     esp, 8
;DEBUGF 1, "lseek: %x\n",eax
        popad

        pushad
        mov     eax, [userdata]
        mov     ebx, [eax + FILE_DISK.fd]
        mov     ecx, [buffer]
        mov     edx, [numsectors]
        mov     edx, [edx]
        imul    edx, [eax + FILE_DISK.Logical]
        mov     eax, SYS_READ
        int     0x80
;DEBUGF 1, "read: %d\n", eax
        popad

        movi    eax, DISK_STATUS_OK
        ret
endp


proc disk_write stdcall, userdata, buffer, startsector:qword, numsectors
        pushad
        mov     eax, dword[startsector + 0]   ; sector lo
        mov     edx, dword[startsector + 4]   ; sector hi
        mov     edx, [userdata]
        mul     [edx + FILE_DISK.Logical]
        mov     ecx, edx
        mov     edx, eax
;DEBUGF 1, "lseek to: %x\n", ecx
        mov     eax, [userdata]
        mov     ebx, [eax + FILE_DISK.fd]
        sub     esp, 8
        mov     esi, esp
        mov     edi, SEEK_SET
        mov     eax, SYS_LLSEEK
        int     0x80
        add     esp, 8
;DEBUGF 1, "lseek: %x\n",eax
        popad

        pushad
        mov     eax, [userdata]
        mov     ebx, [eax + FILE_DISK.fd]
        mov     ecx, [buffer]
        mov     edx, [numsectors]
        mov     edx, [edx]
        imul    edx, [eax + FILE_DISK.Logical]
        mov     eax, SYS_WRITE
        int     0x80
;DEBUGF 1, "write: %d\n", eax
        popad

        movi    eax, DISK_STATUS_OK
        ret
endp


; int querymedia(void* userdata, DISKMEDIAINFO* info);
proc disk_querymedia stdcall, hd_data, mediainfo
        mov     ecx, [mediainfo]
        mov     edx, [hd_data]
        mov     [ecx + DISKMEDIAINFO.Flags], 0
        mov     eax, [edx + FILE_DISK.Logical]
        mov     [ecx + DISKMEDIAINFO.SectorSize], eax
        mov     eax, [edx + FILE_DISK.Sectors]
        mov     dword [ecx + DISKMEDIAINFO.Capacity], eax
        mov     dword [ecx + DISKMEDIAINFO.Capacity + 4], 0
        
        movi    eax, DISK_STATUS_OK
        ret
endp


__malloc:
        push    ecx edx
        ccall   _malloc, eax
        pop     edx ecx
        ret


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


__free:
        ccall   _free, eax
        ret


proc kernel_free base
        mov     eax, [base]
        call    free
        xor     eax, eax
        ret
endp


put_board:
        pushad
        mov     eax, SYS_WRITE
        mov     ebx, STDOUT
        push    ecx
        mov     ecx, esp
        mov     edx, 1
        int     0x80
        pop     ecx
        popad
ret

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
disk_functions:
        dd disk_functions_end - disk_functions
        dd 0
        dd 0
        dd disk_querymedia
        dd disk_read
        dd disk_write
        dd 0
        dd 0
disk_functions_end:

disk_name db 'hd0',0
IncludeIGlobals


section '.bss' writeable align 16
file_disk FILE_DISK
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

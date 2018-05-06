format ELF

__DEBUG__ = 1
__DEBUG_LEVEL__ = 1

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

struct FS_FUNCTIONS
        Free            dd ?
        Size            dd ?
        ReadFile        dd ?
        ReadFolder      dd ?
        CreateFile      dd ?
        WriteFile       dd ?
        SetFileEnd      dd ?
        GetFileInfo     dd ?
        SetFileInfo     dd ?
        Run             dd ?
        Delete          dd ?
        CreateFolder    dd ?
ends

purge section,mov,add,sub
section '.text' executable align 16


;uint32_t kos_time_to_epoch(uint8_t *time);
public kos_time_to_epoch
kos_time_to_epoch:
        push    ebx esi edi ebp

        mov     esi, [esp + 0x14]
        call    fsCalculateTime
        add     eax, 978307200  ; epoch to 2001.01.01

        pop     ebp edi esi ebx
        ret


;void *kos_fuse_init(int fd);
public kos_fuse_init
kos_fuse_init:
        push    ebx esi edi ebp

        mov     [pg_data.pages_free], (128*1024*1024)/0x1000

        mov     eax, [esp + 0x14]
        mov     [fd], eax

        mov     [file_disk.Size], 65536
        stdcall disk_add, disk_functions, disk_name, file_disk, DISK_NO_INSERT_NOTIFICATION
        mov     [disk], eax
        stdcall disk_media_changed, [disk], 1

        mov     eax, [disk]
        cmp     [eax + DISK.NumPartitions], 0
        jnz     .done
        mov     eax, SYS_WRITE
        mov     ebx, STDOUT
        mov     ecx, msg_no_partition
        mov     edx, msg_no_partition.size
        int     0x80
        xor     eax, eax
  .done:
        mov     [fs_struct], eax

        pop     ebp edi esi ebx
        ret


;char *kos_fuse_readdir(const char *path, off_t offset)
public kos_fuse_readdir
kos_fuse_readdir:
;DEBUGF 1, '#kos_fuse_readdir\n'
        push    ebx esi edi ebp

        mov     edx, sf70_params
        mov     dword[edx + 0x00], 1
        mov     eax, [esp + 0x18]       ; offset
        mov     [edx + 0x04], eax
        mov     dword[edx + 0x08], 1    ; cp866
        mov     dword[edx + 0x0c], 100
        mov     dword[edx + 0x10], sf70_buffer
        mov     eax, [esp + 0x14]       ; path
        mov     byte[edx + 0x14], 0
        mov     [edx + 0x15], eax

        mov     ebx, sf70_params
        pushad  ; file_system_lfn writes here
        call    file_system_lfn
        popad
        pop     ebp edi esi ebx
        mov     eax, sf70_buffer
        ret


;void *kos_fuse_getattr(const char *path)
public kos_fuse_getattr
kos_fuse_getattr:
;DEBUGF 1, '#kos_fuse_getattr\n'
        push    ebx esi edi ebp

        mov     edx, sf70_params
        mov     dword[edx + 0x00], 5
        mov     dword[edx + 0x04], 0
        mov     dword[edx + 0x08], 0
        mov     dword[edx + 0x0c], 0
        mov     dword[edx + 0x10], sf70_buffer
        mov     eax, [esp + 0x14]       ; path
        mov     byte[edx + 0x14], 0
        mov     [edx + 0x15], eax

        mov     ebx, sf70_params
        pushad  ; file_system_lfn writes here
        call    file_system_lfn
        popad
        pop     ebp edi esi ebx
        mov     eax, sf70_buffer
        ret


;long *kos_fuse_read(const char *path, char *buf, size_t size, off_t offset)
public kos_fuse_read
kos_fuse_read:
        push    ebx esi edi ebp

        mov     edx, sf70_params
        mov     dword[edx + 0x00], 0
        mov     eax, [esp + 0x20]
        mov     dword[edx + 0x04], eax
        mov     dword[edx + 0x08], 0
        mov     eax, [esp + 0x1c]
        mov     dword[edx + 0x0c], eax
        mov     eax, [esp + 0x18]
        mov     dword[edx + 0x10], eax
        mov     eax, [esp + 0x14]       ; path
        inc     eax     ; skip '/'
        mov     [edx + 0x14], eax

        mov     ebp, [fs_struct]
        mov     ebx, sf70_params
        mov     esi, eax
        push    0
        call    xfs_Read
        pop     eax

        pop     ebp edi esi ebx
        mov     eax, 0
        ret


proc disk_read stdcall, userdata, buffer, startsector:qword, numsectors
        pushad
        mov     eax, dword[startsector + 0]   ; sector lo
        mov     edx, dword[startsector + 4]   ; sector hi
        imul    ecx, eax, 512
        mov     eax, SYS_LSEEK
        mov     ebx, [fd]
        mov     edx, SEEK_SET
        int     0x80
;DEBUGF 1, "lseek: %x\n", eax
        popad

        pushad
        mov     eax, SYS_READ
        mov     ebx, [fd]
        mov     ecx, [buffer]
        mov     edx, [numsectors]
        mov     edx, [edx]
        imul    edx, 512
        int     0x80
;DEBUGF 1, "read: %d\n", eax
        popad

        movi    eax, DISK_STATUS_OK
        ret
endp


proc disk_write stdcall, userdata, buffer, startsector:qword, numsectors
ud2
        pushad
        imul    ecx, eax, 512
        add     ecx, 2048*512
        mov     eax, SYS_LSEEK
        mov     ebx, [fd]
        mov     edx, SEEK_SET
        int     0x80
;DEBUGF 1, "lseek: %x\n", eax
        popad

        pushad
        mov     eax, SYS_WRITE
        mov     ecx, ebx
        mov     ebx, [fd]
        mov     edx, 512
        int     0x80
;DEBUGF 1, "write: %d\n", eax
        popad

        movi    eax, DISK_STATUS_OK
        ret
endp


; int querymedia(void* userdata, DISKMEDIAINFO* info);
proc disk_querymedia stdcall, hd_data, mediainfo
        mov     ecx, [mediainfo]
        mov     [ecx + DISKMEDIAINFO.Flags], 0
        mov     [ecx + DISKMEDIAINFO.SectorSize], 512
        mov     eax, [hd_data]
        mov     eax, dword[eax + FILE_DISK.Size + 0]
        mov     dword [ecx + DISKMEDIAINFO.Capacity], eax
        mov     dword [ecx + DISKMEDIAINFO.Capacity + 4], 0
        
        movi    eax, DISK_STATUS_OK
        ret
endp


malloc:
        push    [alloc_pos]
        add     [alloc_pos], eax
        pop     eax
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




free:
        ret

proc kernel_free blah
        ret
endp


mutex_init:
mutex_lock:
mutex_unlock:
        ret

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
        ret

proc fs_execute
; edx = flags
; ecx -> cmdline
; ebx -> absolute file path
; eax = string length
        ret
endp


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

struct FILE_DISK
        Size dd ?
ends

file_disk FILE_DISK

alloc_pos       dd alloc_base
sf70_params     rd 6
msg_no_partition    db 'no partition detected',0x0a
msg_no_partition.size = $ - msg_no_partition
disk_name db 'hd0',0
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
IncludeIGlobals


section '.bss' writeable align 16
fd      rd 1
disk dd ?
alloc_base      rb 32*1024*1024
fs_struct       rd 1
sf70_buffer     rb 16*1024*1024
IncludeUGlobals
DiskNumber db ?
ChannelNumber db ?
DevErrorCode dd ?
CDSectorAddress dd ?
CDDataBuf_pointer dd ?
DRIVE_DATA: rb 0x4000
cdpos dd ?
cd_appl_data rd 1

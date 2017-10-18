format ELF

__DEBUG__ = 1
__DEBUG_LEVEL__ = 2

include 'proc32.inc'
include 'struct.inc'
include 'macros.inc'
include 'const.inc'
include 'system.inc'
include 'debug-fdo.inc'
include 'disk.inc'
include 'fs_common.inc'

ERROR_SUCCESS        = 0
ERROR_DISK_BASE      = 1
ERROR_UNSUPPORTED_FS = 2
ERROR_UNKNOWN_FS     = 3
ERROR_PARTITION      = 4
ERROR_FILE_NOT_FOUND = 5
ERROR_END_OF_FILE    = 6
ERROR_MEMORY_POINTER = 7
ERROR_DISK_FULL      = 8
ERROR_FS_FAIL        = 9
ERROR_ACCESS_DENIED  = 10
ERROR_DEVICE         = 11
ERROR_OUT_OF_MEMORY  = 12

purge section,mov,add,sub
section '.text' executable align 16

public kos_fuse_init
kos_fuse_init:
        push    ebx esi edi ebp

        mov     eax, [esp + 0x14]
        mov     [fd], eax

        mov     eax, 0
        mov     ebx, mbr_buffer
        mov     ebp, partition
        call    fs_read32_sys

        mov     esi, disk
        mov     [esi + DISK.MediaInfo.SectorSize], 512
        mov     ebp, partition
        mov     [ebp + PARTITION.Disk], esi
        mov     ebx, mbr_buffer
        call    xfs_create_partition
        test    eax, eax
        jnz     @f
        mov     eax, SYS_WRITE
        mov     ebx, STDOUT
        mov     ecx, msg_not_xfs_partition
        mov     edx, msg_not_xfs_partition.size
        int     0x80
    @@:
        mov     [fs_struct], eax

        pop     ebp edi esi ebx
        ret

;char *hello_readdir(const char *path, off_t offset)
public kos_fuse_readdir
kos_fuse_readdir:
        push    ebx esi edi ebp

        mov     edx, sf70_params
        mov     dword[edx + 0x00], 1
        mov     eax, [esp + 0x18]       ; offset
        mov     [edx + 0x04], eax
        mov     dword[edx + 0x08], 1
        mov     dword[edx + 0x0c], 100
        mov     dword[edx + 0x10], sf70_buffer
        mov     eax, [esp + 0x14]       ; path
        inc     eax     ; skip '/'
        mov     [edx + 0x14], eax

        mov     ebp, [fs_struct]
        mov     ebx, sf70_params
        mov     esi, eax
        push    0
        call    xfs_ReadFolder
        pop     eax

        pop     ebp edi esi ebx
        mov     eax, sf70_buffer
        ret


; in: eax = sector, ebx = buffer, ebp = pointer to PARTITION structure
fs_read32_sys:
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
        mov     eax, SYS_READ
        mov     ecx, ebx
        mov     ebx, [fd]
        mov     edx, 512
        int     0x80
;DEBUGF 1, "read: %d\n", eax
        popad

        xor     eax, eax

        ret


fs_read32_app:
        ret


fs_read64_sys:
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
        mov     eax, SYS_READ
        imul    edx, ecx, 512
        mov     ecx, ebx
        mov     ebx, [fd]
        int     0x80
;DEBUGF 1, "read: %d\n", eax
        popad

        xor     eax, eax
        ret


fs_read64_app:
        ret


malloc:
        push    [alloc_pos]
        add     [alloc_pos], eax
        pop     eax
        ret

free:
        ret


kernel_free:
        ret


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


include 'xfs.asm'


section '.data' writeable align 16
include_debug_strings

partition_offset        dd 2048*512
alloc_pos       dd alloc_base
sf70_params     rd 6
msg_not_xfs_partition    db 'not xfs partition',0x0a    ; TODO: return codes, report in C
msg_not_xfs_partition.size = $ - msg_not_xfs_partition
IncludeIGlobals


section '.bss' writeable align 16
mbr_buffer rb 4096*3
fd      rd 1
partition PARTITION
disk DISK
alloc_base      rb 1024*1024
fs_struct       rd 1
sf70_buffer     rb 1024*1024

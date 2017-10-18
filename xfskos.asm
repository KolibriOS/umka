;    lib
;
;    Copyright (C) 2012-2013,2016 Ivan Baravy (dunkaist)
;
;    This program is free software: you can redistribute it and/or modify
;    it under the terms of the GNU General Public License as published by
;    the Free Software Foundation, either version 3 of the License, or
;    (at your option) any later version.
;
;    This program is distributed in the hope that it will be useful,
;    but WITHOUT ANY WARRANTY; without even the implied warranty of
;    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
;    GNU General Public License for more details.
;
;    You should have received a copy of the GNU General Public License
;    along with this program.  If not, see <http://www.gnu.org/licenses/>.

format ELF

__DEBUG__ = 1
__DEBUG_LEVEL__ = 2

include 'proc32.inc'
include 'struct.inc'
include 'macros.inc'
include 'const.inc'
include 'system.inc'
;include 'fdo.inc'
include 'debug-fdo.inc'
include 'disk.inc'
;include 'fs_lfn.inc'
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


;public _start
_start:

;        pushfd
;        pop     eax
;        or      eax, 1 SHL 18   ; Alignment Check flag
;        push    eax
;        popfd

        pop     eax
        cmp     eax, 2
        jz      .params_ok
  .few_arguments:
        mov     eax, SYS_WRITE
        mov     ebx, STDOUT
        mov     ecx, msg_few_args
        mov     edx, msg_few_args.size
        int     0x80
        jmp     .error_quit
  .params_ok:
        pop     eax
        pop     ebx
        mov     [filename], ebx
        mov     eax, SYS_OPEN
        mov     ecx, O_RDONLY OR O_LARGEFILE
        int     0x80
        cmp     eax, -2 ; FIXME RETURN VALUE
        je      .file_not_found
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
        jz      .not_xfs_partition
        mov     [fs_struct], eax

  .next_cmd:
        call    prompt
        call    read_cmd
        mov     eax, dword[cmd_buf]
        cmp     eax, dword 'exit'
        jnz     @f
        jmp     .quit
    @@:
        cmp     ax, word 'ls'
        jnz     @f
        call    cmd_ls
        jmp     .next_cmd
    @@:
  .unknown:
        call    unknown_command
        jmp     .next_cmd

  .quit:
        mov     eax, SYS_EXIT
        xor     ebx, ebx
        int     0x80
   .not_xfs_partition:
        mov     eax, SYS_WRITE
        mov     ebx, STDOUT
        mov     ecx, msg_not_xfs_partition
        mov     edx, msg_not_xfs_partition.size
        int     0x80
        jmp     .error_quit
   .file_not_found:
        mov     eax, SYS_WRITE
        mov     ebx, STDOUT
        mov     ecx, msg_file_not_found
        mov     edx, msg_file_not_found.size
        int     0x80
        stdcall print_filename, [filename]
        jmp     .error_quit
  .error_quit:
        mov     eax, SYS_EXIT
        mov     ebx, 1
        int     0x80


cmd_ls:
        mov     ebp, [fs_struct]
        mov     ebx, sf70_params
        mov     esi, cmd_buf
        mov     edi, esi
        mov     al, ' '
        mov     ecx, 100
        repnz scasb
        mov     [ebx + 0x14], edi
        mov     eax, edi
        sub     eax, esi
        push    eax
        mov     al, 0x0a
        repnz scasb
        mov     byte[edi-1], 0
        call    xfs_ReadFolder
        pop     eax

        mov     eax, sf70_buffer
        mov     ecx, [eax + 4]  ; read bdfes
        add     eax, 0x20       ; skip header
  .next_bdfe:
        lea     edx, [eax + 0x28]
        push    eax ecx
        stdcall print_filename, edx
        pop     ecx eax
        add     eax, 304
        dec     ecx
        jnz     .next_bdfe

        ret


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


unknown_command:
        pushad
        mov     eax, SYS_WRITE
        mov     ebx, STDOUT
        mov     ecx, msg_unknown_command
        mov     edx, msg_unknown_command.size
        int     0x80
        popad
        ret


prompt:
        pushad
        mov     eax, SYS_WRITE
        mov     ebx, STDOUT
        mov     ecx, endl_prompt
        mov     edx, 2
        int     0x80
        popad
        ret

read_cmd:
        pushad
        mov     eax, SYS_READ
        mov     ebx, STDIN
        mov     ecx, cmd_buf
        mov     edx, 4096
        int     0x80
        popad
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
        ret


mutex_lock:
        ret


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


proc print_filename _filename
        stdcall strlen, [_filename]
        mov     byte[edi - 1], 0x0a
        inc     edx
        mov     ecx, [_filename]
        mov     eax, SYS_WRITE
        mov     ebx, STDOUT
        int     0x80
        ret
endp


proc strlen _str
        mov     edi, [_str]
        mov     ecx, -1
        xor     eax, eax
        repnz   scasb
        not     ecx
        dec     ecx
        mov     edx, ecx
        ret
endp


include 'xfs.asm'


section '.data' writeable align 16
msg_few_args             db 'usage: xfskos image [offset]',0x0a
msg_few_args.size        = $ - msg_few_args
msg_file_not_found       db 'file not found: '
msg_file_not_found.size  = $ - msg_file_not_found
msg_unknown_command      db 'unknown command',0x0a
msg_unknown_command.size = $ - msg_unknown_command
msg_not_xfs_partition    db 'not xfs partition',0x0a
msg_not_xfs_partition.size = $ - msg_not_xfs_partition
endl_prompt db '> '

include_debug_strings

partition_offset        dd 2048*512
alloc_pos       dd alloc_base
sf70_params     dd 1, 0, 1, 100, sf70_buffer, -1


section '.bss' writeable align 16
mbr_buffer rb 4096*3
filename        rd 1
fd      rd 1
cmd_buf                 rb 4096
partition PARTITION
disk DISK
alloc_base      rb 1024*1024
fs_struct       rd 1
sf70_buffer     rb 1024*1024

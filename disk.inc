;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;                                                              ;;
;; Copyright (C) KolibriOS team 2011-2015. All rights reserved. ;;
;; Distributed under terms of the GNU General Public License    ;;
;;                                                              ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

$Revision: 6917 $

; =============================================================================
; ================================= Constants =================================
; =============================================================================
; Error codes for callback functions.
DISK_STATUS_OK              = 0 ; success
DISK_STATUS_GENERAL_ERROR   = -1; if no other code is suitable
DISK_STATUS_INVALID_CALL    = 1 ; invalid input parameters
DISK_STATUS_NO_MEDIA        = 2 ; no media present
DISK_STATUS_END_OF_MEDIA    = 3 ; end of media while reading/writing data
DISK_STATUS_NO_MEMORY       = 4 ; insufficient memory for driver operation
; Driver flags. Represent bits in DISK.DriverFlags.
DISK_NO_INSERT_NOTIFICATION = 1
; Media flags. Represent bits in DISKMEDIAINFO.Flags.
DISK_MEDIA_READONLY = 1

; If too many partitions are detected,there is probably an error on the disk.
; 256 partitions should be enough for any reasonable use.
; Also, the same number is limiting the number of MBRs to process; if
; too many MBRs are visible,there probably is a loop in the MBR structure.
MAX_NUM_PARTITIONS = 256

; This structure holds information on a medium.
; Objects with this structure are allocated by the kernel as a part of the DISK
; structure and are filled by a driver in the 'querymedia' callback.
struct  DISKMEDIAINFO
        Flags           dd ?
; Combination of DISK_MEDIA_* bits.
        SectorSize      dd ?
; Size of the sector.
        Capacity        dq ?
; Size of the media in sectors.
ends

; This structure represents the disk cache. To follow the old implementation,
; there are two distinct caches for a disk, one for "system" data,and the other
; for "application" data.
struct  DISKCACHE
; The following fields are inherited from data32.inc:cache_ideX.
        pointer         dd ?
        data_size       dd ?    ; unused
        data            dd ?
        sad_size        dd ?
        search_start    dd ?
        sector_size_log dd ?
ends

; This structure represents a disk device and its media for the kernel.
; This structure is allocated by the kernel in the 'disk_add' function,
; freed in the 'disk_dereference' function.
struct  DISK
; Fields of disk object
        Next            dd ?
        Prev            dd ?
; All disk devices are linked in one list with these two fields.
; Head of the list is the 'disk_list' variable.
        Functions       dd ?
; Pointer to the 'DISKFUNC' structure with driver functions.
        Name            dd ?
; Pointer to the string used for accesses through the global filesystem.
        UserData        dd ?
; This field is passed to all callback functions so a driver can decide which
; physical device is addressed.
        DriverFlags     dd ?
; Bitfield. Currently only DISK_NO_INSERT_NOTIFICATION bit is defined.
; If it is set, the driver will never issue 'disk_media_changed' notification
; with argument set to true, so the kernel must try to detect media during
; requests from the file system.
        RefCount        dd ?
; Count of active references to this structure. One reference is kept during
; the lifetime of the structure between 'disk_add' and 'disk_del'.
; Another reference is taken during any filesystem operation for this disk.
; One reference is added if media is inserted.
; The structure is destroyed when the reference count decrements to zero:
; this usually occurs in 'disk_del', but can be delayed to the end of last
; filesystem operation, if one is active.
        MediaLock       MUTEX
; Lock to protect the MEDIA structure. See the description after
; 'disk_list_mutex' for the locking strategy.
; Fields of media object
        MediaInserted   db ?
; 0 if media is not inserted, nonzero otherwise.
        MediaUsed       db ?
; 0 if media fields are not used, nonzero otherwise. If .MediaRefCount is
; nonzero, this field is nonzero too; however, when .MediaRefCount goes
; to zero, there is some time interval during which media object is still used.
                        dw ? ; padding
; The following fields are not valid unless either .MediaInserted is nonzero
; or they are accessed from a code which has obtained the reference when
; .MediaInserted was nonzero.
        MediaRefCount   dd ?
; Count of active references to the media object. One reference is kept during
; the lifetime of the media between two calls to 'disk_media_changed'.
; Another reference is taken during any filesystem operation for this media.
; The callback 'closemedia' is called when the reference count decrements to
; zero: this usually occurs in 'disk_media_changed', but can be delayed to the
; end of the last filesystem operation, if one is active.
        MediaInfo       DISKMEDIAINFO
; This field keeps information on the current media.
        NumPartitions   dd ?
; Number of partitions on this media.
        Partitions      dd ?
; Pointer to array of .NumPartitions pointers to PARTITION structures.
        cache_size      dd ?
; inherited from cache_ideX_size
        CacheLock       MUTEX
; Lock to protect both caches.
        SysCache        DISKCACHE
        AppCache        DISKCACHE
; Two caches for the disk.
ends

; This structure represents one partition for the kernel. This is a base
; template, the actual contents after common fields is determined by the
; file system code for this partition.
struct  PARTITION
        FirstSector     dq ?
; First sector of the partition.
        Length          dq ?
; Length of the partition in sectors.
        Disk            dd ?
; Pointer to parent DISK structure.
        FSUserFunctions dd ?
; Handlers for the sysfunction 70h. This field is a pointer to the following
; array. The first dword is pointer to disconnect handler.
; The first dword is a number of supported subfunctions, other dwords
; point to handlers of corresponding subfunctions.
; ...fs-specific data may follow...
ends

; This is an external structure, it represents an entry in the partition table.
struct  PARTITION_TABLE_ENTRY
        Bootable        db ?
; 80h = bootable partition, 0 = non-bootable partition, other values = invalid
        FirstHead       db ?
        FirstSector     db ?
        FirstTrack      db ?
; Coordinates of first sector in CHS.
        Type            db ?
; Partition type, one of predefined constants. 0 = empty, several types denote
; extended partition (see process_partition_table_entry), we are not interested
; in other values.
        LastHead        db ?
        LastSector      db ?
        LastTrack       db ?
; Coordinates of last sector in CHS.
        FirstAbsSector  dd ?
; Coordinate of first sector in LBA.
        Length          dd ?
; Length of the partition in sectors.
ends

; GUID Partition Table Header, UEFI 2.6, Table 18
struct GPTH
        Signature                rb 8
; 'EFI PART'
        Revision                 dd ?
; 0x00010000
        HeaderSize               dd ?
; Size of this header in bytes, must fit to one sector.
        HeaderCRC32              dd ?
; Set this field to zero, compute CRC32 via 0xEDB88320, compare.
        Reserved                 dd ?
; Must be zero.
        MyLBA                    dq ?
; LBA of the sector containing this GPT header.
        AlternateLBA             dq ?
; LBA of the sector containing the other GPT header.
; AlternateLBA of Primary GPTH points to Backup one and vice versa.
        FirstUsableLBA           dq ?
; Only sectors between first and last UsableLBA may form partitions
        LastUsableLBA            dq ?
        DiskGUID                 rb 16
; Globally Unique IDentifier
        PartitionEntryLBA        dq ?
; First LBA of Partition Entry Array.
; Length in bytes is computed as a product of two following fields.
        NumberOfPartitionEntries dd ?
; Actual number of partitions depends on the contents of Partition Entry Array.
; A partition entry is unused if zeroed.
        SizeOfPartitionEntry     dd ?   ; in bytes
        PartitionEntryArrayCRC32 dd ?
; Same CRC as for GPT header.
ends

; GPT Partition Entry, UEFI 2.6, Table 19
struct GPE
        PartitionTypeGUID       rb 16
        UniquePartitionGUID     rb 16
        StartingLBA             dq ?
        EndingLBA               dq ?
; Length in sectors is EndingLBA - StartingLBA + 1.
        Attributes              dq ?
        PartitionName           rb 72
ends

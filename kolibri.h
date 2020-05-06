#ifndef KOLIBRI_H_INCLUDED
#define KOLIBRI_H_INCLUDED

#include <stdio.h>
#include <stdint.h>
#include <stddef.h>

#define BDFE_LEN_CP866 304
#define BDFE_LEN_UNICODE 560

typedef struct {
    uint32_t left, top, right, bottom;
} rect_t;

typedef struct {
    uint32_t left, top, width, height;
} box_t;

typedef struct {
    uint32_t cpu_usage;
    uint16_t window_stack_position;
    uint16_t window_stack_value;
    uint16_t pad;
    char process_name[12];
    uint32_t memory_start;
    uint32_t used_memory;
    uint32_t pid;
    box_t box;
    uint16_t slot_state;
    uint16_t pad2;
    box_t client_box;
    uint8_t wnd_state;
    uint8_t pad3[1024-71];
} __attribute__((packed)) process_information_t;

typedef struct {
    uint32_t frame, grab, work_3d_dark, work_3d_light, grab_text, work,
             work_button, work_button_text, work_text, work_graph;
} system_colors_t;

typedef enum {
    DEFAULT_ENCODING,
    CP866,
    UTF16,
    UTF8,
    INVALID_ENCODING,
} fs_enc_t;

typedef enum {
    F70 = 70,
    F80 = 80,
} f70or80_t;

enum {
    ERROR_SUCCESS,
    ERROR_DISK_BASE,
    ERROR_UNSUPPORTED_FS,
    ERROR_UNKNOWN_FS,
    ERROR_PARTITION,
    ERROR_FILE_NOT_FOUND,
    ERROR_END_OF_FILE,
    ERROR_MEMORY_POINTER,
    ERROR_DISK_FULL,
    ERROR_FS_FAIL,
    ERROR_ACCESS_DENIED,
    ERROR_DEVICE,
    ERROR_OUT_OF_MEMORY,
};

typedef struct lhead lhead_t;

struct lhead {
    lhead_t *next;
    lhead_t *prev;
};

typedef struct {
    lhead_t  wait_list;
    uint32_t count;
} mutex_t;

typedef struct {
    uint32_t flags;
    uint32_t sector_size;
    uint64_t capacity;  // in sectors
} diskmediainfo_t;

typedef struct {
    uintptr_t   pointer;
    uint32_t    data_size;
    uintptr_t   data;
    uint32_t    sad_size;
    uint32_t    search_start;
    uint32_t    sector_size_log;
} disk_cache_t;

typedef struct {
    uint64_t first_sector;
    uint64_t length;    // in sectors
    void *disk;
    void *fs_user_functions;
} partition_t;

typedef struct disk_t disk_t;

typedef struct {
    uint32_t  strucsize;
    void (*close)(void *userdata) __attribute__((__stdcall__));
    void (*closemedia)(void *userdata) __attribute__((__stdcall__));
    int (*querymedia)(void *userdata, diskmediainfo_t *info) __attribute__((__stdcall__));
    int (*read)(void *userdata, void *buffer, off_t startsector, size_t *numsectors) __attribute__((__stdcall__));
    int (*write)(void *userdata, void *buffer, off_t startsector, size_t *numsectors) __attribute__((__stdcall__));
    int (*flush)(void *userdata) __attribute__((__stdcall__));
    unsigned int (*adjust_cache_size)(void *userdata, uint32_t suggested_size) __attribute__((__stdcall__));
} diskfunc_t;

struct disk_t {
    disk_t *next;
    disk_t *prev;
    diskfunc_t *functions;
    const char *name;
    void *userdata;
    uint32_t driver_flags;
    uint32_t ref_count;
    mutex_t media_lock;
    uint8_t media_inserted;
    uint8_t media_used;
    uint16_t padding;
    uint32_t media_ref_count;
    diskmediainfo_t media_info;
    uint32_t num_partitions;
    partition_t **partitions;
    uint32_t cache_size;
    mutex_t cache_lock;
    disk_cache_t sys_cache;
    disk_cache_t app_cache;
};

typedef struct {
    uint32_t attr;
    uint32_t enc;
    uint32_t ctime;
    uint32_t cdate;
    uint32_t atime;
    uint32_t adate;
    uint32_t mtime;
    uint32_t mdate;
    uint64_t size;
    char name[0];
} bdfe_t;

typedef struct {
    uint32_t status;
    uint32_t count;
} f7080ret_t;

typedef struct {
    uint32_t sf;
    uint64_t offset;
    uint32_t count;
    void *buf;
    union {
        struct {
            uint8_t zero;
            const char *path;
        } __attribute__((packed)) f70;
        struct {
            uint32_t path_encoding;
            const char *path;
        } f80;
    } u;
} __attribute__((packed)) f7080s0arg_t;

typedef struct {
    uint32_t sf;
    uint32_t offset;
    uint32_t encoding;
    uint32_t size;
    void *buf;
    union {
        struct {
            uint8_t zero;
            const char *path;
        } __attribute__((packed)) f70;
        struct {
            uint32_t path_encoding;
            const char *path;
        } f80;
    } u;
} __attribute__((packed)) f7080s1arg_t;

typedef struct {
    uint32_t version;
    uint32_t cnt;
    uint32_t total_cnt;
    uint32_t zeroed[5];
    bdfe_t bdfes[0];
} f7080s1info_t;

typedef struct {
    uint32_t sf;
    uint32_t reserved1;
    uint32_t flags;
    uint32_t reserved2;
    void *buf;
    union {
        struct {
            uint8_t zero;
            const char *path;
        } __attribute__((packed)) f70;
        struct {
            uint32_t path_encoding;
            const char *path;
        } f80;
    } u;
} __attribute__((packed)) f7080s5arg_t;

#define KF_READONLY 0x01
#define KF_HIDDEN   0x02
#define KF_SYSTEM   0x04
#define KF_LABEL    0x08
#define KF_FOLDER   0x10
#define KF_ATTR_CNT 5

#define HASH_SIZE 32
typedef struct {
    uint8_t hash[HASH_SIZE];
    uint8_t opaque[1024-HASH_SIZE];
} hash_context;

typedef struct {
    uint32_t edi;
    uint32_t esi;
    uint32_t ebp;
    uint32_t esp;
    uint32_t ebx;
    uint32_t edx;
    uint32_t ecx;
    uint32_t eax;
} pushad_t;

#define NET_TYPE_ETH 1
#define NET_TYPE_SLIP 2

// Link state
#define ETH_LINK_DOWN    0x0    // Link is down
#define ETH_LINK_UNKNOWN 0x1    // There could be an active link
#define ETH_LINK_FD      0x2    // full duplex flag
#define ETH_LINK_10M     0x4    // 10 mbit
#define ETH_LINK_100M    0x8    // 100 mbit
#define ETH_LINK_1G      0xc    // gigabit

typedef struct {
    uint32_t device_type;   // type field
    uint32_t mtu;           // Maximal Transmission Unit
    char *name;             // ptr to 0 terminated string

    void *unload;           // ptrs to driver functions
    void *reset;
    void *transmit;

    uint64_t bytes_tx;      // statistics, updated by the driver
    uint64_t bytes_rx;
    uint32_t packets_tx;
    uint32_t packets_rx;

    uint32_t link_state;    // link state (0 = no link)
    uint32_t hwacc;         // bitmask stating enabled HW accelerations (offload
                            // engines)
} net_device_t; // NET_DEVICE

void kos_init(void);
void i40(void);
uint32_t kos_time_to_epoch(uint32_t *time);

void *disk_add(diskfunc_t *disk, const char *name, void *userdata, uint32_t flags)  __attribute__((__stdcall__));
void *disk_media_changed(diskfunc_t *disk, int inserted) __attribute__((__stdcall__));
void disk_del(disk_t *disk)  __attribute__((__stdcall__));

void hash_oneshot(void *ctx, void *data, size_t len);

void xfs_user_functions(void);
void ext_user_functions(void);
void fat_user_functions(void);
void ntfs_user_functions(void);

static inline void kos_enable_acpi() {
    __asm__ __inline__ __volatile__ (
        "pushad;"
        "call   enable_acpi;"
        "popad"
        :
        :
        : "memory", "cc");
}

static inline void kos_stack_init() {
    __asm__ __inline__ __volatile__ (
        "pushad;"
        "call   stack_init;"
        "popad"
        :
        :
        : "memory", "cc");
}

static inline int32_t kos_net_add_device(net_device_t *dev) {
    int32_t dev_num;
    __asm__ __inline__ __volatile__ (
        "call   net_add_device"
        : "=a"(dev_num)
        : "b"(dev)
        : "ecx", "edx", "esi", "edi", "memory", "cc");

    return dev_num;
}

void coverage_begin(void);
void coverage_end(void);

extern uint32_t *kos_lfb_base;
extern uint16_t *kos_win_stack;
extern uint16_t *kos_win_pos;
extern uint32_t kos_acpi_ssdt_cnt;
extern uint8_t **kos_acpi_ssdt_base;
extern size_t *kos_acpi_ssdt_size;
extern disk_t disk_list;

#endif

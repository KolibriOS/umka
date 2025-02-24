/*
    SPDX-License-Identifier: GPL-2.0-or-later

    UMKa - User-Mode KolibriOS developer tools

    Copyright (C) 2017-2023  Ivan Baravy <dunkaist@gmail.com>
    Copyright (C) 2021  Magomed Kostoev <mkostoevr@yandex.ru>
*/

#ifndef UMKA_H_INCLUDED
#define UMKA_H_INCLUDED

#include <assert.h>
#include <inttypes.h>
#include <signal.h>
#include <stdatomic.h>
#include <stddef.h>
#include <string.h>
#include <sys/types.h>
#include <time.h>

#define UMKA_PATH_MAX 4096
#define UMKA_DEFAULT_THREAD_STACK_SIZE 0x10000
#define RAMDISK_MAX_LEN (2880*512)

// TODO: Cleanup
#ifndef _WIN32
#include <signal.h> // for irq0: siginfo_t
#else
typedef void siginfo_t;
#endif

#define STDCALL __attribute__((__stdcall__))

enum {
    UMKA_RUNNING_NEVER,
    UMKA_RUNNING_NOT_YET,
    UMKA_RUNNING_YES,
};

struct umka_ctx {
    int booted;
    atomic_int running;
};

#define KEYBOARD_MODE_ASCII     0
#define KEYBOARD_MODE_SCANCODES 1

#define UMKA_IRQ_BASE 0x20  // skip CPU exceptions
#define UMKA_SIGNAL_IRQ SIGSYS
#define UMKA_IRQ_MOUSE 14
#define UMKA_IRQ_NETWORK 15

enum kos_lang {
    KOS_LANG_EN = 1,
    KOS_LANG_FIRST = KOS_LANG_EN,
    KOS_LANG_FI,
    KOS_LANG_GE,
    KOS_LANG_RU,
    KOS_LANG_FR,
    KOS_LANG_ET,
    KOS_LANG_UA,
    KOS_LANG_IT,
    KOS_LANG_BE,
    KOS_LANG_SP,
    KOS_LANG_CA,
    KOS_LANG_LAST = KOS_LANG_CA
};

#define KOS_TSTATE_RUNNING        0
#define KOS_TSTATE_RUN_SUSPENDED  1
#define KOS_TSTATE_WAIT_SUSPENDED 2
#define KOS_TSTATE_ZOMBIE         3
#define KOS_TSTATE_TERMINATING    4
#define KOS_TSTATE_WAITING        5
#define KOS_TSTATE_FREE           9

#define KOS_LAYOUT_SIZE 128

#define BDFE_LEN_CP866 304
#define BDFE_LEN_UNICODE 560

#define EVENT_REDRAW     0x0001
#define EVENT_KEY        0x0002
#define EVENT_BUTTON     0x0004
#define EVENT_BACKGROUND 0x0010
#define EVENT_MOUSE      0x0020
#define EVENT_IPC        0x0040
#define EVENT_NETWORK    0x0080
#define EVENT_DEBUG      0x0100
#define EVENT_NETWORK2   0x0200
#define EVENT_EXTENDED   0x0400

#define KOS_APP_BASE 0
#define MENUET01_MAGIC "MENUET01"

struct app_hdr {
    union {
        struct {
            char magic[8];
            uint32_t version;
            uint32_t start;
            uint32_t i_end;
            uint32_t mem_size;
            uint32_t stack_top;
            uint32_t i_param;
            uint32_t i_icon;
        } menuet;
    };
};

struct point16s {
    int16_t y, x;
};

struct mouse_state {
    uint8_t bl_held:1;
    uint8_t br_held:1;
    uint8_t bm_held:1;
    uint8_t b4_held:1;
    uint8_t b5_held:1;
};

struct mouse_events {
    uint8_t bl_pressed:1;
    uint8_t br_pressed:1;
    uint8_t bm_pressed:1;
    uint8_t :5;
    uint8_t vscroll_used:1;
    uint8_t bl_released:1;
    uint8_t br_released:1;
    uint8_t bm_released:1;
    uint8_t :4;
    uint8_t hscroll_used:1;
    uint8_t bl_dbclick:1;
    uint8_t :6;
};

struct mouse_state_events {
    struct mouse_state st;
    struct mouse_events ev;
};

typedef struct {
    uint32_t left, top, right, bottom;
} rect_t;

typedef struct {
    uint32_t left, top, width, height;
} box_t;

typedef struct {
    uint32_t dr0, dr1, dr2, dr3, dr7;
} dbg_regs_t;

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

static_assert(sizeof(process_information_t) == 0x400,
              "must be 0x400 bytes long");

typedef struct wdata {
    box_t box;
    uint32_t cl_workarea;
    uint32_t cl_titlebar;
    uint32_t cl_frames;
    uint8_t z_modif;
    uint8_t fl_wstate;
    uint8_t fl_wdrawn;
    uint8_t fl_redraw;
    box_t clientbox;
    uint8_t *shape;
    uint32_t shape_scale;
    char *caption;
    uint8_t caption_encoding;
    uint8_t pad0[3];
    box_t saved_box;
    void *cursor;
    void *temp_cursor;
    uint32_t draw_bgr_x;
    uint32_t draw_bgr_y;
    rect_t draw_data;
    struct appdata *thread;
    uint8_t pad1[12];
} __attribute__((packed)) wdata_t;

static_assert(sizeof(struct wdata) == 0x80, "must be 0x80 bytes long");

typedef struct {
    uint32_t frame;
    uint32_t grab;
    uint32_t work_3d_dark;
    uint32_t work_3d_light;
    uint32_t grab_text;
    uint32_t work;
    uint32_t work_button;
    uint32_t work_button_text;
    uint32_t work_text;
    uint32_t work_graph;
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
    KOS_ERROR_SUCCESS,
    KOS_ERROR_DISK_BASE,
    KOS_ERROR_UNSUPPORTED_FS,
    KOS_ERROR_UNKNOWN_FS,
    KOS_ERROR_PARTITION,
    KOS_ERROR_FILE_NOT_FOUND,
    KOS_ERROR_END_OF_FILE,
    KOS_ERROR_MEMORY_POINTER,
    KOS_ERROR_DISK_FULL,
    KOS_ERROR_FS_FAIL,
    KOS_ERROR_ACCESS_DENIED,
    KOS_ERROR_DEVICE,
    KOS_ERROR_OUT_OF_MEMORY,
};

typedef struct lhead lhead_t;

struct lhead {
    void *next;
    void *prev;
};

typedef struct {
    lhead_t  wait_list;
    uint32_t count;
} mutex_t;

struct board_get_ret {
    uint32_t value;
    uint32_t status;
};

typedef mutex_t rwsem_t;

typedef struct {
    uint32_t flags;
    uint32_t sector_size;
    uint64_t capacity;  // in sectors
    uint32_t last_session_sector;
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
    STDCALL void (*close)(void *userdata);
    STDCALL void (*closemedia)(void *userdata);
    STDCALL int (*querymedia)(void *userdata, diskmediainfo_t *info);
    STDCALL int (*read)(void *userdata, void *buffer, off_t startsector,
                        size_t *numsectors);
    STDCALL int (*write)(void *userdata, void *buffer, off_t startsector,
                         size_t *numsectors);
    STDCALL int (*flush)(void *userdata);
    STDCALL unsigned int (*adjust_cache_size)(void *userdata,
                                              size_t suggested_size);
    STDCALL int (*loadtray)(void *userdata, int flags);
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
    char name[0x777];  // how to handle this properly? FIXME
} bdfe_t;

typedef struct {
    int32_t status;
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
    bdfe_t bdfes[];
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

typedef struct {
    uint32_t sf;
    uint32_t flags;
    char *params;
    uint32_t reserved1;
    uint32_t reserved2;
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
} __attribute__((packed)) f7080s7arg_t;

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

#define NET_TYPE_ETH  1
#define NET_TYPE_SLIP 2

// Link state
#define ETH_LINK_DOWN    0x0    // Link is down
#define ETH_LINK_UNKNOWN 0x1    // There could be an active link
#define ETH_LINK_FD      0x2    // full duplex flag
#define ETH_LINK_10M     0x4    // 10 mbit
#define ETH_LINK_100M    0x8    // 100 mbit
#define ETH_LINK_1G      0xc    // gigabit

// Ethernet protocol numbers
#define ETHER_PROTO_ARP           0x0608
#define ETHER_PROTO_IPv4          0x0008
#define ETHER_PROTO_IPv6          0xDD86
#define ETHER_PROTO_PPP_DISCOVERY 0x6388
#define ETHER_PROTO_PPP_SESSION   0x6488

// Internet protocol numbers
#define IP_PROTO_IP   0
#define IP_PROTO_ICMP 1
#define IP_PROTO_TCP  6
#define IP_PROTO_UDP  17
#define IP_PROTO_RAW  255

// IP options
#define IP_TOS     1
#define IP_TTL     2
#define IP_HDRINCL 3

// PPP protocol numbers
#define PPP_PROTO_IPv4     0x2100
#define PPP_PROTO_IPV6     0x5780
#define PPP_PROTO_ETHERNET 666

// Protocol family
#define AF_INET4  AF_INET

struct net_device;

#define NET_BUFFER_SIZE 0x800

typedef struct {
        void *next;     // pointer to next frame in list
        void *prev;     // pointer to previous frame in list
        struct net_device *device;  // ptr to NET_DEVICE structure
        uint32_t type;  // encapsulation type: e.g. Ethernet
        size_t length;  // size of encapsulated data
        size_t offset;  // offset to actual data (24 bytes for default frame)
        uint8_t data[];
} net_buff_t;

struct net_device {
    uint32_t device_type;   // type field
    uint32_t mtu;           // Maximal Transmission Unit
    char *name;             // ptr to 0 terminated string

    // ptrs to driver functions
    STDCALL void (*unload) (void);
    STDCALL void (*reset) (void);
    STDCALL int (*transmit) (net_buff_t *);

    uint32_t link_state;    // link state (0 = no link)
    uint32_t hwacc;         // bitmask stating enabled HW accelerations (offload
                            // engines)
    uint64_t bytes_tx;      // statistics, updated by the driver
    uint64_t bytes_rx;

    uint32_t packets_tx;
    uint32_t packets_tx_err;
    uint32_t packets_tx_drop;
    uint32_t packets_tx_ovr;

    uint32_t packets_rx;
    uint32_t packets_rx_err;
    uint32_t packets_rx_drop;
    uint32_t packets_rx_ovr;
}; // NET_DEVICE

struct eth_device {
    struct net_device net;
    uint8_t mac[6];
};

typedef struct {
    uint32_t ip;
    uint8_t mac[6];
    uint16_t status;
    uint16_t ttl;
} arp_entry_t;

typedef struct acpi_node acpi_node_t;
struct acpi_node {
    uint32_t name;
    int32_t refcount;
    acpi_node_t *parent;
    acpi_node_t *children;
    acpi_node_t *next;
    int32_t type;
};

typedef struct {
    acpi_node_t node;
    uint64_t value;
} kos_node_integer_t;

typedef struct {
    acpi_node_t node;
    acpi_node_t **list;
    size_t el_cnt;
} kos_node_package_t;

static inline void
umka_osloop(void) {
    __asm__ __inline__ __volatile__ (
        "pusha;"
        "call   kos_osloop;"
        "popa"
        :
        :
        : "memory", "cc");
}

void
irq0(int signo, siginfo_t *info, void *context);

struct umka_ctx *
umka_init(int running);

void
umka_close(struct umka_ctx *ctx);

void
umka_boot(void);

void
i40(void);

time_t
kos_time_to_epoch(uint32_t *time);

STDCALL disk_t *
disk_add(diskfunc_t *disk, const char *name, void *userdata, uint32_t flags);

STDCALL void *
disk_media_changed(disk_t *disk, int inserted);

STDCALL void
disk_del(disk_t *disk);

void
hash_oneshot(void *ctx, void *data, size_t len);

extern atomic_int idle_scheduled;
extern atomic_int os_scheduled;

extern uint8_t xfs_user_functions[];
extern uint8_t ext_user_functions[];
extern uint8_t fat_user_functions[];
extern uint8_t exfat_user_functions[];
extern uint8_t ntfs_user_functions[];
extern uint8_t iso9660_user_functions[];

extern char kos_ramdisk[RAMDISK_MAX_LEN];

disk_t *
kos_ramdisk_init(void);

STDCALL void
kos_set_mouse_data(uint32_t btn_state, int32_t xmoving, int32_t ymoving,
                   int32_t vscroll, int32_t hscroll);

STDCALL net_buff_t *
kos_net_buff_alloc(size_t size);

struct idt_entry {
    uint16_t addr_lo;
    uint16_t segment;
    uint16_t flags;
    uint16_t addr_hi;
};

typedef int (*hw_int_handler_t)(void*);

extern struct idt_entry kos_idts[];

STDCALL void
kos_attach_int_handler(int irq, hw_int_handler_t handler, void *userdata);

void
kos_irq_serv_irq10(void);

struct ret_create_event {
    uint32_t event; // (=0 => fail)
    uint32_t uid;
};

#define KOS_ACPI_NODE_Uninitialized 1
#define KOS_ACPI_NODE_Integer       2
#define KOS_ACPI_NODE_String        3
#define KOS_ACPI_NODE_Buffer        4
#define KOS_ACPI_NODE_Package       5
#define KOS_ACPI_NODE_OpRegionField 6
#define KOS_ACPI_NODE_IndexField    7
#define KOS_ACPI_NODE_BankField     8
#define KOS_ACPI_NODE_Device        9

extern acpi_node_t *kos_acpi_root;

typedef struct {
    int pew[0x100];
} amlctx_t;

struct pci_dev {
    struct pci_dev *next;
    size_t bus;
    size_t dev;
    size_t fun;
    void *acpi;
    struct pci_dev *children;
    struct pci_dev *parent;
    void *prt;
    size_t is_bridge;
    size_t vendor_id;
    size_t device_id;
    size_t int_pin;
    size_t gsi;
};

extern struct pci_dev *kos_pci_root;

void
kos_acpi_aml_init(void);

STDCALL void
kos_aml_attach(acpi_node_t *parent, acpi_node_t *node);

STDCALL void
kos_acpi_fill_pci_irqs(void *ctx);

STDCALL amlctx_t*
kos_acpi_aml_new_thread(void);

STDCALL acpi_node_t*
kos_aml_alloc_node(int32_t type);

STDCALL acpi_node_t*
kos_aml_constructor_integer(void);

STDCALL acpi_node_t*
kos_aml_constructor_package(size_t el_cnt);

STDCALL acpi_node_t*
kos_acpi_lookup_node(acpi_node_t *root, char *name);

STDCALL void
kos_acpi_print_tree(void *ctx);

#define MAX_PCI_DEVICES 256

extern void *kos_acpi_dev_data;
extern size_t kos_acpi_dev_size;
extern void *kos_acpi_dev_next;

void kos_eth_input(void *buf);

STDCALL void*
kos_kernel_alloc(size_t len);

STDCALL void
kos_pci_walk_tree(struct pci_dev *node,
                  STDCALL void* (*test)(struct pci_dev *node, void *arg),
                  STDCALL void* (*clbk)(struct pci_dev *node, void *arg),
                  void *arg);

typedef struct {
    uint32_t value;
    uint32_t errorcode;
} f75ret_t;

typedef struct {
    uint32_t eax;
    uint32_t ebx;
} f76ret_t;

static inline void
umka_stack_init(void) {
    __asm__ __inline__ __volatile__ (
        "pusha;"
        "call   kos_stack_init;"
        "popa"
        :
        :
        : "memory", "cc");
}

static inline int32_t
kos_net_add_device(struct net_device *dev) {
    int32_t dev_num;
    __asm__ __inline__ __volatile__ (
        "call   net_add_device"
        : "=a"(dev_num)
        : "b"(dev)
        : "ecx", "edx", "esi", "edi", "memory", "cc");

    return dev_num;
}

STDCALL void
kos_window_set_screen(ssize_t left, ssize_t top, ssize_t right, ssize_t bottom,
                      ssize_t proc);

typedef struct {
    int32_t x;
    int32_t y;
    size_t width;
    size_t height;
    size_t bits_per_pixel;
    size_t vrefresh;
    void *current_lfb;
    size_t lfb_pitch;

    rwsem_t win_map_lock;
    uint8_t *win_map;
    size_t win_map_pitch;
    size_t win_map_size;

    void *modes;
    void *ddev;
    void *connector;
    void *crtc;

    void *cr_list_next;
    void *cr_list_prev;

    void *cursor;

    void *init_cursor;
    void *select_cursor;
    void *show_cursor;
    void *move_cursor;
    void *restore_cursor;
    void *disable_mouse;
    size_t mask_seqno;
    void *check_mouse;
    void *check_m_pixel;

    size_t bytes_per_pixel;

    void *put_pixel;
    void *put_rect;
    void *put_image;
    void *put_line;
    void *get_pixel;
    void *get_rect;
    void *get_image;
    void *get_line;
} __attribute__((packed)) display_t;

extern display_t kos_display;

typedef struct {
    uint64_t addr;
    uint64_t size;
    uint32_t type;
} e820entry_t;

#define MAX_MEMMAP_BLOCKS 32

typedef struct {
    uint8_t bpp;    // bits per pixel
    uint16_t pitch; // scanline length
    uint8_t pad1[5];
    uint16_t vesa_mode;
    uint16_t x_res;
    uint16_t y_res;
    uint8_t pad2[6];
    uint32_t bank_switch;   // Vesa 1.2 pm bank switch
    void *lfb;  // Vesa 2.0 LFB address
    uint8_t mtrr;   // 0 or 1: enable MTRR graphics acceleration
    uint8_t launcher_start; // 0 or 1: start the first app (right now it's
                            // LAUNCHER) after kernel is loaded
    uint8_t debug_print;    // if nonzero, duplicates debug output to the screen
    uint8_t dma;    // DMA write: 1=yes, 2=no
    uint8_t pci_data[8];
    uint8_t pad3[8];
    uint8_t shutdown_type;  // see sysfn 18.9
    uint8_t pad4[15];
    uint32_t apm_entry; // entry point of APM BIOS
    uint16_t apm_version;   // BCD
    uint16_t apm_flags;
    uint8_t pad5[8];
    uint16_t apm_code_32;
    uint16_t apm_code_16;
    uint16_t apm_data_16;
    uint8_t rd_load_from;   // Device to load ramdisk from, RD_LOAD_FROM_*
    uint8_t pad6[1];
    uint16_t kernel_restart;
    uint16_t sys_disk;  // Device to mount on /sys/, see loader_doc.txt for details
    void *acpi_rsdp;
    char syspath[0x17];
    void *devicesdat_data;
    size_t devicesdat_size;
    uint8_t bios_hd_cnt;    // number of BIOS hard disks
    uint8_t bios_hd[0x80];  // BIOS hard disks
    size_t memmap_block_cnt;    // available physical memory map: number of blocks
    e820entry_t memmap_blocks[MAX_MEMMAP_BLOCKS];
    uint8_t acpi_usage;
} __attribute__((packed)) boot_data_t;

extern uint8_t kos_msg_board_data[];
extern uint32_t kos_msg_board_count;

extern boot_data_t kos_boot;

void
umka_cli(void);

void
umka_sti(void);

void
set_eflags_tf(uint32_t tf);

#define COVERAGE_TABLE_SIZE (512*1024)

struct coverage_branch {
    uint64_t to_cnt;
    uint64_t from_cnt;
};

extern struct coverage_branch coverage_table[];
extern uint8_t coverage_begin[];
extern uint8_t coverage_end[];

typedef struct appobj_t appobj_t;

struct appobj_t {
    uint32_t magic;
    void *destroy;  // internal destructor
    appobj_t *fd;   // next object in list
    appobj_t *bk;   // prev object in list
    uint32_t pid;   // owner id
};

typedef struct {
    uint32_t magic;
    void *destroy;  // internal destructor
    appobj_t *fd;   // next object in list
    appobj_t *bk;   // prev object in list
    uint32_t pid;   // owner id
    uint32_t id;    // event uid
    uint32_t state; // internal flags
    uint32_t code;
    uint32_t pad[5];
} event_t;

typedef struct {
    lhead_t list;
    lhead_t thr_list;
    mutex_t heap_lock;
    void *heap_base;
    void *heap_top;
    uint32_t mem_used;
    void *dlls_list_ptr;
    void *pdt_0_phys;
    void *pdt_1_phys;
    void *io_map_0;
    void *io_map_1;

    void *ht_lock;
    void *ht_free;
    void *ht_next;
    void *htab[(1024-18*4)/4];
    void *pdt_0[1024];
} proc_t;

static_assert(sizeof(proc_t) == 0x1400, "must be 0x1400 bytes long");

typedef struct appdata {
    char app_name[11];
    uint8_t pad1[5];

    lhead_t list;                  // +16
    proc_t *process;               // +24
    void *fpu_state;               // +28
    void *exc_handler;             // +32
    uint32_t except_mask;          // +36
    void *pl0_stack;               // +40
    uint32_t pad2;                 // +44
    event_t *fd_ev;                // +48
    event_t *bk_ev;                // +52
    appobj_t *fd_obj;              // +56
    appobj_t *bk_obj;              // +60
    void *saved_esp;               // +64
    uint32_t io_map[2];            // +68
    uint32_t dbg_state;            // +76
    char *cur_dir;                 // +80
    uint32_t wait_timeout;         // +84
    void *saved_esp0;              // +88
    uint32_t wait_begin;           // +92
    int (*wait_test)(void);        // +96
    void *wait_param;              // +100
    void *tls_base;                // +104
    uint32_t event_mask;           // +108
    uint32_t tid;                  // +112
    uint32_t pad3;                 // +116
    uint32_t pad4;                 // +120
    uint8_t state;                 // +124
    uint8_t wnd_number;            // +125
    uint16_t pad5;                 // +126
    struct wdata *window;          // +128
    uint32_t pad6;                 // +132
    uint32_t pad7;                 // +136
    uint32_t counter_sum;          // +140
    uint32_t pad8[4];              // +144
    uint32_t *ipc_start;           // +160
    size_t ipc_size;               // +164
    uint32_t occurred_events;      // +168
    uint32_t debugger_slot;        // +172
    uint32_t terminate_protection; // +176
    uint8_t keyboard_mode;         // +180
    uint8_t pad9[3];               // +181
    char *exec_params;             // +184
    void *dbg_event_mem;           // +188
    dbg_regs_t dbg_regs;           // +192
    uint32_t pad10;                // +212
    uint32_t pad11[4];             // +216
    uint32_t priority;             // +232
    lhead_t in_schedule;           // +236
    uint32_t counter_add;          // +244
    uint32_t cpu_usage;            // +248
    uint32_t pad12;                // +252
} appdata_t;

static_assert(sizeof(struct appdata) == 256, "must be 0x100 bytes long");

extern uint8_t kos_redraw_background;
extern size_t kos_task_count;
extern wdata_t kos_window_data[];
extern appdata_t kos_slot_base[];
extern uint32_t kos_current_process;
extern appdata_t *kos_current_slot;
extern uint32_t kos_current_slot_idx;
extern void umka_do_change_task(appdata_t *new);
extern void scheduler_add_thread(void);
extern void find_next_task(void);
extern uint8_t kos_lfb_base[];
extern uint16_t kos_win_stack[];
extern uint16_t kos_win_pos[];
extern uint32_t kos_acpi_ssdt_cnt;
extern uint8_t *kos_acpi_ssdt_base[];
extern size_t kos_acpi_ssdt_size[];
extern void *acpi_ctx;
extern uint32_t kos_acpi_usage;
extern uint32_t kos_acpi_node_alloc_cnt;
extern uint32_t kos_acpi_node_free_cnt;
extern uint32_t kos_acpi_count_nodes(void *ctx) STDCALL;
extern disk_t disk_list;
extern uint8_t kos_key_count;
extern uint8_t kos_key_buff[120*2 + 2*2];
extern uint8_t kos_keyboard_mode;
extern uint32_t kos_syslang;
extern uint32_t kos_keyboard;

static inline void
umka_scheduler_add_thread(appdata_t *thread, int32_t priority) {
    __asm__ __inline__ __volatile__ (
        "call   do_change_thread"
        :
        : "c"(priority),
          "d"(thread)
        : "memory", "cc");
}

#define KOS_MAX_PRIORITY    0 // highest, used for kernel tasks
#define KOS_USER_PRIORITY   1 // default
#define KOS_IDLE_PRIORITY   2 // lowest, only IDLE thread goes here
#define KOS_NR_SCHED_QUEUES 3 // MUST equal IDLE_PRIORYTY + 1

#define SCHEDULE_ANY_PRIORITY 0
#define SCHEDULE_HIGHER_PRIORITY 1

extern appdata_t *kos_scheduler_current[KOS_NR_SCHED_QUEUES];

void
i40_asm(uint32_t eax,
        uint32_t ebx,
        uint32_t ecx,
        uint32_t edx,
        uint32_t esi,
        uint32_t edi,
        uint32_t ebp,
        uint32_t *eax_out,
        uint32_t *ebx_out);

static inline void
umka_i40(pushad_t *regs) {
    i40_asm(regs->eax,
            regs->ebx,
            regs->ecx,
            regs->edx,
            regs->esi,
            regs->edi,
            regs->ebp,
            &regs->eax,
            &regs->ebx);
}

static inline struct ret_create_event
kos_create_event(void *data, uint32_t flags) {
    struct ret_create_event ret;
    __asm__ __inline__ __volatile__ (
        "push   ebx esi edi ebp;"
        "call   _kos_create_event;"
        "pop    ebp edi esi ebx"
        : "=a"(ret.event),
          "=d"(ret.uid)
        : "S"(data),
          "c"(flags)
        : "memory", "cc");
    return ret;
}

static inline void*
kos_destroy_event(void *event, uint32_t uid) {
    void *ret;
    __asm__ __inline__ __volatile__ (
        "push   ebx;"
        "push   esi;"
        "push   edi;"
        "push   ebp;"
        "call   _kos_destroy_event;"
        "pop    ebp;"
        "pop    edi;"
        "pop    esi;"
        "pop    ebx"
        : "=a"(ret)
        : "a"(event),
          "b"(uid)
        : "memory", "cc");
    return ret;
}

static inline void
kos_wait_event(void *event, uint32_t uid) {
    __asm__ __inline__ __volatile__ (
        "push   ebx;"
        "push   esi;"
        "push   edi;"
        "push   ebp;"
        "call   _kos_wait_event;"
        "pop    ebp;"
        "pop    edi;"
        "pop    esi;"
         "pop   ebx"
        :
        : "a"(event),
          "b"(uid)
        : "memory", "cc");
}

typedef uint32_t (*wait_test_t)(void);

static inline void *
kos_wait_events(wait_test_t wait_test, void *wait_param) {
    void *res;
    __asm__ __inline__ __volatile__ (
        "push   ebx;"
        "push   esi;"
        "push   edi;"
        "push   ebp;"
        "call   _kos_wait_events;"
        "pop    ebp;"
        "pop    edi;"
        "pop    esi;"
        "pop    ebx"
        : "=a"(res)
        : "c"(wait_param),
          "d"(wait_test)
        : "memory", "cc");
    return res;
}

static inline int32_t
umka_fs_execute(const char *filename) {
// edx Flags
// ecx Commandline
// ebx Absolute file path
// eax String length
    int32_t result;
    __asm__ __inline__ __volatile__ (
        "push   ebp;"
        "call   kos_fs_execute;"
        "pop    ebp"
        : "=a"(result)
        : "a"(strlen(filename)),
          "b"(filename),
          "c"(NULL),
          "d"(0)
        : "memory");
    return result;
}

typedef void (*kos_thread_t)(void);

static inline size_t
kos_new_sys_threads(uint32_t flags, kos_thread_t entry, void *stack_top) {
    size_t tid;
    __asm__ __inline__ __volatile__ (
        "push   ebx;"
        "push   esi;"
        "push   edi;"
        "push   ebp;"
        "call   _kos_new_sys_threads;"
        "pop    ebp;"
        "pop    edi;"
        "pop    esi;"
        "pop    ebx"
        : "=a"(tid)
        : "b"(flags),
          "c"(entry),
          "d"(stack_top)
        : "memory", "cc");
    return tid;
}

static inline size_t
umka_new_sys_threads(uint32_t flags, void (*entry)(void *), void *stack_top,
                     void *arg, const char *app_name) {
    kos_thread_t entry_noparam = (kos_thread_t)entry;
    size_t tid = kos_new_sys_threads(flags, entry_noparam, stack_top);
    appdata_t *t = kos_slot_base + tid;
    strncpy(t->app_name, app_name, 11);
    *(void**)((char*)t->saved_esp0-12) = arg;   // param for the thread
    // -12 here because in UMKa, unlike real hardware, we don't switch between
    // kernel and userspace, i.e. stack structure is different
    return tid;
}

static inline void
kos_enable_acpi(void) {
    __asm__ __inline__ __volatile__ (
        "pusha;"
        "call   enable_acpi;"
        "popa"
        :
        :
        : "memory", "cc");
}

static inline void
kos_acpi_call_name(void *ctx, const char *name) {
    __asm__ __inline__ __volatile__ (
        "pusha;"
        "push   %[name];"
        "push   %[ctx];"
        "call   acpi.call_name;"
        "popa"
        :
        : [ctx] "r"(ctx), [name] "r"(name)
        : "memory", "cc");
}

static inline void
umka_sys_draw_window(size_t x, size_t xsize, size_t y, size_t ysize,
                     uint32_t color, int has_caption, int client_relative,
                     int fill_workarea, int gradient_fill, int movable,
                     uint32_t style, const char *caption) {
    __asm__ __inline__ __volatile__ (
        "call   i40"
        :
        : "a"(0),
          "b"((x << 16) + xsize),
          "c"((y << 16) + ysize),
          "d"((gradient_fill << 31) + (!fill_workarea << 30)
              + (client_relative << 29) + (has_caption << 28) + (style << 24)
              + color),
          "S"(!movable << 24),
          "D"(caption)
        : "memory");
}

static inline void
umka_sys_set_pixel(size_t x, size_t y, uint32_t color, int invert) {
    __asm__ __inline__ __volatile__ (
        "call   i40"
        :
        : "a"(1),
          "b"(x),
          "c"(y),
          "d"((invert << 24) + color)
        : "memory");
}

static inline uint32_t
umka_get_key(void) {
    uint32_t key;
    __asm__ __inline__ __volatile__ (
        "call   i40"
        : "=a"(key)
        : "a"(2)
        : "memory");
    return key;
}

static inline void
umka_sys_write_text(size_t x, size_t y, uint32_t color, int asciiz,
                    int fill_background, int font_and_encoding,
                    int draw_to_buffer, int scale_factor, const char *string,
                    size_t length, uintptr_t background_color_or_buffer) {
    __asm__ __inline__ __volatile__ (
        "call   i40"
        :
        : "a"(4),
          "b"((x << 16) + y),
          "c"((asciiz << 31) + (fill_background << 30)
              + (font_and_encoding << 28) + (draw_to_buffer << 27)
              + (scale_factor << 24) + color),
          "d"(string),
          "S"(length),
          "D"(background_color_or_buffer)
        : "memory");
}

static inline void
umka_sys_csleep(size_t cs) {
    __asm__ __inline__ __volatile__ (
        "call   i40"
        :
        : "a"(5),
          "b"(cs)
        : "memory");
}

static inline void
umka_sys_put_image(void *image, size_t xsize, size_t ysize, size_t x,
                   size_t y) {
    __asm__ __inline__ __volatile__ (
        "call   i40"
        :
        : "a"(7),
          "b"(image),
          "c"((xsize << 16) + ysize),
          "d"((x << 16) + y)
        : "memory");
}

static inline void
umka_sys_button(size_t x, size_t xsize, size_t y, size_t ysize,
                size_t button_id, int draw_button, int draw_frame,
                uint32_t color) {
    __asm__ __inline__ __volatile__ (
        "call   i40"
        :
        : "a"(8),
          "b"((x << 16) + xsize),
          "c"((y << 16) + ysize),
          "d"((!draw_button << 30) + (!draw_frame << 29) + button_id),
          "S"(color)
        : "memory");
}

static inline void
umka_sys_process_info(int32_t pid, void *param) {
    __asm__ __inline__ __volatile__ (
        "call   i40"
        :
        : "a"(9),
          "b"(param),
          "c"(pid)
        : "memory");
}

static inline uint32_t
umka_sys_wait_for_event(void) {
    uint32_t event;
    __asm__ __inline__ __volatile__ (
        "call   i40"
        : "=a"(event)
        : "a"(10)
        : "memory");
    return event;
}

static inline uint32_t
umka_sys_check_for_event(void) {
    uint32_t event;
    __asm__ __inline__ __volatile__ (
        "call   i40"
        : "=a"(event)
        : "a"(11)
        : "memory");
    return event;
}

static inline void
umka_sys_window_redraw(int begin_end) {
    __asm__ __inline__ __volatile__ (
        "call   i40"
        :
        : "a"(12),
          "b"(begin_end)
        : "memory");
}

static inline void
umka_sys_draw_rect(size_t x, size_t xsize, size_t y, size_t ysize,
                   uint32_t color, int gradient) {
    __asm__ __inline__ __volatile__ (
        "call   i40"
        :
        : "a"(13),
          "b"((x << 16) + xsize),
          "c"((y << 16) + ysize),
          "d"((gradient << 31) + color)
        : "memory");
}

static inline void
umka_sys_get_screen_size(uint32_t *xsize, uint32_t *ysize) {
    uint32_t xysize;
    __asm__ __inline__ __volatile__ (
        "call   i40"
        : "=a"(xysize)
        : "a"(14)
        : "memory");
    *xsize = (xysize >> 16) + 1;
    *ysize = (xysize & 0xffffu) + 1;
}

static inline void
umka_sys_bg_set_size(uint32_t xsize, uint32_t ysize) {
    __asm__ __inline__ __volatile__ (
        "call   i40"
        :
        : "a"(15),
          "b"(1),
          "c"(xsize),
          "d"(ysize)
        : "memory");
}

static inline void
umka_sys_bg_put_pixel(uint32_t offset, uint32_t color) {
    __asm__ __inline__ __volatile__ (
        "call   i40"
        :
        : "a"(15),
          "b"(2),
          "c"(offset),
          "d"(color)
        : "memory");
}

static inline void
umka_sys_bg_redraw(void) {
    __asm__ __inline__ __volatile__ (
        "call   i40"
        :
        : "a"(15),
          "b"(3)
        : "memory");
}

static inline void
umka_sys_bg_set_mode(uint32_t mode) {
    __asm__ __inline__ __volatile__ (
        "call   i40"
        :
        : "a"(15),
          "b"(4),
          "c"(mode)
        : "memory");
}

static inline void
umka_sys_bg_put_img(void *image, size_t offset, size_t size) {
    __asm__ __inline__ __volatile__ (
        "call   i40"
        :
        : "a"(15),
          "b"(5),
          "c"(image),
          "d"(offset),
          "S"(size)
        : "memory");
}

static inline void *
umka_sys_bg_map(void) {
    void *addr;
    __asm__ __inline__ __volatile__ (
        "call   i40"
        : "=a"(addr)
        : "a"(15),
          "b"(6)
        : "memory");
    return addr;
}

static inline uint32_t
umka_sys_bg_unmap(void *addr) {
    uint32_t status;
    __asm__ __inline__ __volatile__ (
        "call   i40"
        : "=a"(status)
        : "a"(15),
          "b"(7),
          "c"(addr)
        : "memory");
    return status;
}

static inline void
umka_sys_set_mouse_pos_screen(struct point16s new_pos) {
    __asm__ __inline__ __volatile__ (
        "call   i40"
        :
        : "a"(18),
          "b"(19),
          "c"(4),
          "d"(new_pos)
        : "memory");
}

static inline int
umka_sys_set_keyboard_layout(int type, void *layout) {
    int status;
    __asm__ __inline__ __volatile__ (
        "call   i40"
        : "=a"(status)
        : "a"(21),
          "b"(2),
          "c"(type),
          "d"(layout)
        : "memory");
    return status;
}

static inline int
umka_sys_set_keyboard_lang(enum kos_lang lang_id) {
    int status;
    __asm__ __inline__ __volatile__ (
        "call   i40"
        : "=a"(status)
        : "a"(21),
          "b"(2),
          "c"(9),
          "d"(lang_id)
        : "memory");
    return status;
}

static inline int
umka_sys_set_system_lang(enum kos_lang lang_id) {
    int status;
    __asm__ __inline__ __volatile__ (
        "call   i40"
        : "=a"(status)
        : "a"(21),
          "b"(5),
          "c"(lang_id)
        : "memory");
    return status;
}

static inline void
umka_sys_get_keyboard_layout(int type, void *layout) {
    int status;
    __asm__ __inline__ __volatile__ (
        "call   i40"
        : "=a"(status)
        : "a"(26),
          "b"(2),
          "c"(type),
          "d"(layout)
        : "memory");
}

static inline int
umka_sys_get_keyboard_lang(void) {
    int status;
    __asm__ __inline__ __volatile__ (
        "call   i40"
        : "=a"(status)
        : "a"(26),
          "b"(2),
          "c"(9)
        :);
    return status;
}

static inline int
umka_sys_get_system_lang(void) {
    int status;
    __asm__ __inline__ __volatile__ (
        "call   i40"
        : "=a"(status)
        : "a"(26),
          "b"(5)
        :);
    return status;
}

static inline void
umka_sys_set_cwd(const char *dir) {
    __asm__ __inline__ __volatile__ (
        "call   i40"
        :
        : "a"(30),
          "b"(1),
          "c"(dir)
        : "memory");
}

static inline void
umka_sys_get_cwd(const char *buf, size_t len) {
    __asm__ __inline__ __volatile__ (
        "call   i40"
        :
        : "a"(30),
          "b"(2),
          "c"(buf),
          "d"(len)
        : "memory");
}

static inline struct point16s
umka_sys_get_mouse_pos_screen(void) {
    struct point16s pos;
    __asm__ __inline__ __volatile__ (
        "call   i40"
        : "=a"(pos)
        : "a"(37),
          "b"(0)
        :);
    return pos;
}

static inline struct point16s
umka_sys_get_mouse_pos_window(void) {
    struct point16s pos;
    __asm__ __inline__ __volatile__ (
        "call   i40"
        : "=a"(pos)
        : "a"(37),
          "b"(1)
        :);
    return pos;
}

static inline struct mouse_state
umka_sys_get_mouse_buttons_state(void) {
    struct mouse_state mouse;
    __asm__ __inline__ __volatile__ (
        "call   i40"
        : "=a"(mouse)
        : "a"(37),
          "b"(2)
        :);
    return mouse;
}

static inline struct mouse_state_events
umka_sys_get_mouse_buttons_state_events(void) {
    union {uint32_t x; struct mouse_state_events m;} u;
    __asm__ __inline__ __volatile__ (
        "call   i40"
        : "=a"(u.x)
        : "a"(37),
          "b"(3)
        :);
    return u.m;
}

static inline void*
umka_sys_load_cursor_from_file(const char *fname) {
    void *handle;
    __asm__ __inline__ __volatile__ (
        "call   i40"
        : "=a"(handle)
        : "a"(37),
          "b"(4),
          "c"(fname),
          "d"(0)
        :);
    return handle;
}

static inline void*
umka_sys_load_cursor_from_mem(void *data) {
    void *handle;
    __asm__ __inline__ __volatile__ (
        "call   i40"
        : "=a"(handle)
        : "a"(37),
          "b"(4),
          "c"(data),
          "d"(1)
        :);
    return handle;
}

static inline void*
umka_sys_set_cursor(void *handle) {
    void *prev;
    __asm__ __inline__ __volatile__ (
        "call   i40"
        : "=a"(prev)
        : "a"(37),
          "b"(5),
          "c"(handle)
        : "memory");
    return prev;
}

static inline void
umka_sys_draw_line(size_t x, size_t xend, size_t y, size_t yend, uint32_t color,
                   int invert) {
    __asm__ __inline__ __volatile__ (
        "call   i40"
        :
        : "a"(38),
          "b"((x << 16) + xend),
          "c"((y << 16) + yend),
          "d"((invert << 24) + color)
        : "memory");
}

static inline void
umka_sys_display_number(int is_pointer, int base, int digits_to_display,
                        int is_qword, int show_leading_zeros,
                        int number_or_pointer, size_t x, size_t y,
                        uint32_t color, int fill_background, int font,
                        int draw_to_buffer, int scale_factor,
                        uintptr_t background_color_or_buffer) {
    __asm__ __inline__ __volatile__ (
        "call   i40"
        :
        : "a"(47),
          "b"(is_pointer + (base << 8) + (digits_to_display << 16)
              + (is_qword << 30) + (show_leading_zeros << 31)),
          "c"(number_or_pointer),
          "d"((x << 16) + y),
          "S"(color + (fill_background << 30) + (font << 28)
              + (draw_to_buffer << 27) + (scale_factor << 24)),
          "D"(background_color_or_buffer)
        : "memory");
}

static inline void
umka_sys_set_button_style(int style) {
    __asm__ __inline__ __volatile__ (
        "call   i40"
        :
        : "a"(48),
          "b"(1),
          "c"(style)
        : "memory");
}

static inline void
umka_sys_set_window_colors(void *colors) {
    __asm__ __inline__ __volatile__ (
        "call   i40"
        :
        : "a"(48),
          "b"(2),
          "c"(colors),
          "d"(40)
        : "memory");
}

static inline void
umka_sys_get_window_colors(void *colors) {
    __asm__ __inline__ __volatile__ (
        "call   i40"
        :
        : "a"(48),
          "b"(3),
          "c"(colors),
          "d"(40)
        : "memory");
}

static inline uint32_t
umka_sys_get_skin_height(void) {
    uint32_t skin_height;
    __asm__ __inline__ __volatile__ (
        "call   i40"
        : "=a"(skin_height)
        : "a"(48),
          "b"(4)
        : "memory");
    return skin_height;
}

static inline void
umka_sys_get_screen_area(rect_t *wa) {
    uint32_t eax, ebx;
    __asm__ __inline__ __volatile__ (
        "call   i40"
        : "=a"(eax),
          "=b"(ebx)
        : "a"(48),
          "b"(5)
        : "memory");
    wa->left   = eax >> 16;
    wa->right  = eax & 0xffffu;
    wa->top    = ebx >> 16;
    wa->bottom = ebx & 0xffffu;
}

static inline void
umka_sys_set_screen_area(rect_t *wa) {
    uint32_t ecx, edx;
    ecx = (wa->left << 16) + wa->right;
    edx = (wa->top << 16) + wa->bottom;
    __asm__ __inline__ __volatile__ (
        "call   i40"
        :
        : "a"(48),
          "b"(6),
          "c"(ecx),
          "d"(edx)
        : "memory");
}

static inline void
umka_sys_get_skin_margins(rect_t *wa) {
    uint32_t eax, ebx;
    __asm__ __inline__ __volatile__ (
        "call   i40"
        : "=a"(eax),
          "=b"(ebx)
        : "a"(48),
          "b"(7)
        : "memory");
    wa->left   = eax >> 16;
    wa->right  = eax & 0xffffu;
    wa->top    = ebx >> 16;
    wa->bottom = ebx & 0xffffu;
}

static inline int32_t
umka_sys_set_skin(const char *path) {
    int32_t status;
    __asm__ __inline__ __volatile__ (
        "call   i40"
        : "=a"(status)
        : "a"(48),
          "b"(8),
          "c"(path)
        : "memory");
    return status;
}

static inline int
umka_sys_get_font_smoothing(void) {
    int type;
    __asm__ __inline__ __volatile__ (
        "call   i40"
        : "=a"(type)
        : "a"(48),
          "b"(9)
        : "memory");
    return type;
}

static inline void
umka_sys_set_font_smoothing(int type) {
    __asm__ __inline__ __volatile__ (
        "call   i40"
        :
        : "a"(48),
          "b"(10),
          "c"(type)
        : "memory");
}

static inline int
umka_sys_get_font_size(void) {
    uint32_t size;
    __asm__ __inline__ __volatile__ (
        "call   i40"
        : "=a"(size)
        : "a"(48),
          "b"(11)
        : "memory");
    return size;
}

static inline void
umka_sys_set_font_size(uint32_t size) {
    __asm__ __inline__ __volatile__ (
        "call   i40"
        :
        : "a"(48),
          "b"(12),
          "c"(size)
        : "memory");
}

static inline void
umka_sys_board_put(char c) {
    __asm__ __inline__ __volatile__ (
        "call   i40"
        :
        : "a"(63),
          "b"(1),
          "c"(c)
        : "memory");
}

static inline struct board_get_ret
umka_sys_board_get(void) {
    struct board_get_ret ret;
    __asm__ __inline__ __volatile__ (
        "call   i40"
        : "=a"(ret.value),
          "=b"(ret.status)
        : "a"(63),
          "b"(2)
        : "memory");
    return ret;
}

static inline void
umka_sys_put_image_palette(void *image, size_t xsize, size_t ysize,
                           size_t x, size_t y, size_t bpp, void *palette,
                           size_t row_offset) {
    pushad_t regs = { 0 };
    regs.eax = 65;
    regs.ebx = (uintptr_t)image;
    regs.ecx = (xsize << 16) + ysize;
    regs.edx = (x << 16) + y;
    regs.esi = bpp;
    regs.edi = (uintptr_t)palette;
    regs.ebp = row_offset;
    umka_i40(&regs);
}

static inline void
umka_sys_set_keyboard_mode(int mode) {
    __asm__ __inline__ __volatile__ (
        "call   i40"
        :
        : "a"(66),
          "b"(1),
          "c"(mode)
        : "memory");
}

static inline int
umka_sys_get_keyboard_mode(void) {
    int mode;
    __asm__ __inline__ __volatile__ (
        "call   i40"
        : "=a"(mode)
        : "a"(66),
          "b"(2)
        : "memory");
    return mode;
}

static inline void
umka_sys_move_window(size_t x, size_t y, ssize_t xsize, ssize_t ysize) {
    __asm__ __inline__ __volatile__ (
        "call   i40"
        :
        : "a"(67),
          "b"(x),
          "c"(y),
          "d"(xsize),
          "S"(ysize)
        : "memory");
}

static inline void *
umka_sys_load_dll(const char *fname) {
    void *table;
    __asm__ __inline__ __volatile__ (
        "call   i40"
        : "=a"(table)
        : "a"(68),
          "b"(19),
          "c"(fname)
        : "memory");
    return table;
}

struct sys_load_file_ret {
    void *fdata;
    size_t fsize;
};

static inline size_t
kos_sys_misc_init_heap(void) {
    size_t heap_size;

    __asm__ __inline__ __volatile__ (
        "call   i40"
        : "=a"(heap_size)
        : "a"(68),
          "b"(11)
        : "memory");

    return heap_size;
}

static inline struct sys_load_file_ret
kos_sys_misc_load_file(const char *fname) {
    struct sys_load_file_ret ret;

    __asm__ __inline__ __volatile__ (
        "call   i40"
        : "=a"(ret.fdata),
          "=d"(ret.fsize)
        : "a"(68),
          "b"(27),
          "c"(fname)
        : "memory");

    return ret;
}

static inline void
umka_sys_lfn(void *f7080sXarg, f7080ret_t *r, f70or80_t f70or80) {
    __asm__ __inline__ __volatile__ (
        "call   i40"
        : "=a"(r->status),
          "=b" (r->count)
        : "a"(f70or80),
          "b"(f7080sXarg)
        : "memory");
}

static inline void
umka_sys_set_window_caption(const char *caption, int encoding) {
    __asm__ __inline__ __volatile__ (
        "call   i40"
        :
        : "a"(71),
          "b"(encoding ? 2 : 1),
          "c"(caption),
          "d"(encoding)
        : "memory");
}

static inline void
umka_sys_blit_bitmap(int operation, int background, int transparent,
                     int client_relative, void *params) {
    __asm__ __inline__ __volatile__ (
        "call   i40"
        :
        : "a"(73),
          "b"((client_relative << 29) + (transparent << 5) + (background << 4)
              + operation),
          "c"(params)
        : "memory");
}

static inline uint32_t
umka_sys_net_get_dev_count(void) {
    uint32_t count;
    __asm__ __inline__ __volatile__ (
        "call   i40"
        : "=a"(count)
        : "a"(74),
          "b"(255)
        : "memory");
    return count;
}

static inline int32_t
umka_sys_net_get_dev_type(uint8_t dev_num) {
    int32_t type;
    __asm__ __inline__ __volatile__ (
        "call   i40"
        : "=a"(type)
        : "a"(74),
          "b"((dev_num << 8) + 0)
        : "memory");
    return type;
}

static inline int32_t
umka_sys_net_get_dev_name(uint8_t dev_num, char *name) {
    int32_t status;
    __asm__ __inline__ __volatile__ (
        "call   i40"
        : "=a"(status)
        : "a"(74),
          "b"((dev_num << 8) + 1),
          "c"(name)
        : "memory");
    return status;
}

static inline int32_t
umka_sys_net_dev_reset(uint8_t dev_num) {
    int32_t status;
    __asm__ __inline__ __volatile__ (
        "call   i40"
        : "=a"(status)
        : "a"(74),
          "b"((dev_num << 8) + 2)
        : "memory");
    return status;
}

static inline int32_t
umka_sys_net_dev_stop(uint8_t dev_num) {
    int32_t status;
    __asm__ __inline__ __volatile__ (
        "call   i40"
        : "=a"(status)
        : "a"(74),
          "b"((dev_num << 8) + 3)
        : "memory");
    return status;
}

static inline intptr_t
umka_sys_net_get_dev(uint8_t dev_num) {
    intptr_t dev;
    __asm__ __inline__ __volatile__ (
        "call   i40"
        : "=a"(dev)
        : "a"(74),
          "b"((dev_num << 8) + 4)
        : "memory");
    return dev;
}

static inline uint32_t
umka_sys_net_get_packet_tx_count(uint8_t dev_num) {
    uint32_t count;
    __asm__ __inline__ __volatile__ (
        "call   i40"
        : "=a"(count)
        : "a"(74),
          "b"((dev_num << 8) + 6)
        : "memory");
    return count;
}

static inline uint32_t
umka_sys_net_get_packet_rx_count(uint8_t dev_num) {
    uint32_t count;
    __asm__ __inline__ __volatile__ (
        "call   i40"
        : "=a"(count)
        : "a"(74),
          "b"((dev_num << 8) + 7)
        : "memory");
    return count;
}

static inline uint32_t
umka_sys_net_get_byte_tx_count(uint8_t dev_num) {
    uint32_t count;
    __asm__ __inline__ __volatile__ (
        "call   i40"
        : "=a"(count)
        : "a"(74),
          "b"((dev_num << 8) + 8)
        : "memory");
    return count;
}

static inline uint32_t
umka_sys_net_get_byte_rx_count(uint8_t dev_num) {
    uint32_t count;
    __asm__ __inline__ __volatile__ (
        "call   i40"
        : "=a"(count)
        : "a"(74),
          "b"((dev_num << 8) + 9)
        : "memory");
    return count;
}

static inline uint32_t
umka_sys_net_get_link_status(uint8_t dev_num) {
    uint32_t status;
    __asm__ __inline__ __volatile__ (
        "call   i40"
        : "=a"(status)
        : "a"(74),
          "b"((dev_num << 8) + 10)
        : "memory");
    return status;
}

static inline f75ret_t
umka_sys_net_open_socket(uint32_t domain, uint32_t type, uint32_t protocol) {
    f75ret_t r;
    __asm__ __inline__ __volatile__ (
        "call   i40"
        : "=a"(r.value),
          "=b"(r.errorcode)
        : "a"(75),
          "b"(0),
          "c"(domain),
          "d"(type),
          "S"(protocol)
        : "memory");
    return r;
}

static inline f75ret_t
umka_sys_net_close_socket(uint32_t fd) {
    f75ret_t r;
    __asm__ __inline__ __volatile__ (
        "call   i40"
        : "=a"(r.value),
          "=b"(r.errorcode)
        : "a"(75),
          "b"(1),
          "c"(fd)
        : "memory");
    return r;
}

static inline f75ret_t
umka_sys_net_bind(uint32_t fd, void *sockaddr, size_t sockaddr_len) {
    f75ret_t r;
    __asm__ __inline__ __volatile__ (
        "call   i40"
        : "=a"(r.value),
          "=b"(r.errorcode)
        : "a"(75),
          "b"(2),
          "c"(fd),
          "d"(sockaddr),
          "S"(sockaddr_len)
        : "memory");
    return r;
}

static inline f75ret_t
umka_sys_net_listen(uint32_t fd, uint32_t backlog) {
    f75ret_t r;
    __asm__ __inline__ __volatile__ (
        "call   i40"
        : "=a"(r.value),
          "=b"(r.errorcode)
        : "a"(75),
          "b"(3),
          "c"(fd),
          "d"(backlog)
        : "memory");
    return r;
}

static inline f75ret_t
umka_sys_net_connect(uint32_t fd, void *sockaddr, size_t sockaddr_len) {
    f75ret_t r;
    __asm__ __inline__ __volatile__ (
        "call   i40"
        : "=a"(r.value),
          "=b"(r.errorcode)
        : "a"(75),
          "b"(4),
          "c"(fd),
          "d"(sockaddr),
          "S"(sockaddr_len)
        : "memory");
    return r;
}

static inline f75ret_t
umka_sys_net_accept(uint32_t fd, void *sockaddr, size_t sockaddr_len) {
    f75ret_t r;
    __asm__ __inline__ __volatile__ (
        "call   i40"
        : "=a"(r.value),
          "=b"(r.errorcode)
        : "a"(75),
          "b"(5),
          "c"(fd),
          "d"(sockaddr),
          "S"(sockaddr_len)
        : "memory");
    return r;
}

static inline f75ret_t
umka_sys_net_send(uint32_t fd, void *buf, size_t buf_len, uint32_t flags) {
    f75ret_t r;
    __asm__ __inline__ __volatile__ (
        "call   i40"
        : "=a"(r.value),
          "=b"(r.errorcode)
        : "a"(75),
          "b"(6),
          "c"(fd),
          "d"(buf),
          "S"(buf_len),
          "D"(flags)
        : "memory");
    return r;
}

static inline f75ret_t
umka_sys_net_receive(uint32_t fd, void *buf, size_t buf_len, uint32_t flags) {
    f75ret_t r;
    __asm__ __inline__ __volatile__ (
        "call   i40"
        : "=a"(r.value),
          "=b"(r.errorcode)
        : "a"(75),
          "b"(7),
          "c"(fd),
          "d"(buf),
          "S"(buf_len),
          "D"(flags)
        : "memory");
    return r;
}

static inline f76ret_t
umka_sys_net_eth_read_mac(uint32_t dev_num) {
    f76ret_t r;
    __asm__ __inline__ __volatile__ (
        "call   i40"
        : "=a"(r.eax),
          "=b"(r.ebx)
        : "a"(76),
          "b"((0 << 16) + (dev_num << 8) + 0)
        : "memory");
    return r;
}

// Function 76, Protocol 1 - IPv4, Subfunction 0, Read # Packets sent =
// Function 76, Protocol 1 - IPv4, Subfunction 1, Read # Packets rcvd =

static inline f76ret_t
umka_sys_net_ipv4_get_addr(uint32_t dev_num) {
    f76ret_t r;
    __asm__ __inline__ __volatile__ (
        "call   i40"
        : "=a"(r.eax),
          "=b"(r.ebx)
        : "a"(76),
          "b"((1 << 16) + (dev_num << 8) + 2)
        : "memory");
    return r;
}

static inline f76ret_t
umka_sys_net_ipv4_set_addr(uint32_t dev_num, uint32_t addr) {
    f76ret_t r;
    __asm__ __inline__ __volatile__ (
        "call   i40"
        : "=a"(r.eax),
          "=b"(r.ebx)
        : "a"(76),
          "b"((1 << 16) + (dev_num << 8) + 3),
          "c"(addr)
        : "memory");
    return r;
}

static inline f76ret_t
umka_sys_net_ipv4_get_dns(uint32_t dev_num) {
    f76ret_t r;
    __asm__ __inline__ __volatile__ (
        "call   i40"
        : "=a"(r.eax),
          "=b"(r.ebx)
        : "a"(76),
          "b"((1 << 16) + (dev_num << 8) + 4)
        : "memory");
    return r;
}

static inline f76ret_t
umka_sys_net_ipv4_set_dns(uint32_t dev_num, uint32_t dns) {
    f76ret_t r;
    __asm__ __inline__ __volatile__ (
        "call   i40"
        : "=a"(r.eax),
          "=b"(r.ebx)
        : "a"(76),
          "b"((1 << 16) + (dev_num << 8) + 5),
          "c"(dns)
        : "memory");
    return r;
}

static inline f76ret_t
umka_sys_net_ipv4_get_subnet(uint32_t dev_num) {
    f76ret_t r;
    __asm__ __inline__ __volatile__ (
        "call   i40"
        : "=a"(r.eax),
          "=b"(r.ebx)
        : "a"(76),
          "b"((1 << 16) + (dev_num << 8) + 6)
        : "memory");
    return r;
}

static inline f76ret_t
umka_sys_net_ipv4_set_subnet(uint32_t dev_num, uint32_t subnet) {
    f76ret_t r;
    __asm__ __inline__ __volatile__ (
        "call   i40"
        : "=a"(r.eax),
          "=b"(r.ebx)
        : "a"(76),
          "b"((1 << 16) + (dev_num << 8) + 7),
          "c"(subnet)
        : "memory");
    return r;
}

static inline f76ret_t
umka_sys_net_ipv4_get_gw(uint32_t dev_num) {
    f76ret_t r;
    __asm__ __inline__ __volatile__ (
        "call   i40"
        : "=a"(r.eax),
          "=b"(r.ebx)
        : "a"(76),
          "b"((1 << 16) + (dev_num << 8) + 8)
        : "memory");
    return r;
}

static inline f76ret_t
umka_sys_net_ipv4_set_gw(uint32_t dev_num, uint32_t gw) {
    f76ret_t r;
    __asm__ __inline__ __volatile__ (
        "call   i40"
        : "=a"(r.eax),
          "=b"(r.ebx)
        : "a"(76),
          "b"((1 << 16) + (dev_num << 8) + 9),
          "c"(gw)
        : "memory");
    return r;
}

// Function 76, Protocol 2 - ICMP, Subfunction 0, Read # Packets sent =
// Function 76, Protocol 2 - ICMP, Subfunction 1, Read # Packets rcvd =
// Function 76, Protocol 3 - UDP, Subfunction 0, Read # Packets sent ==
// Function 76, Protocol 3 - UDP, Subfunction 1, Read # Packets rcvd ==
// Function 76, Protocol 4 - TCP, Subfunction 0, Read # Packets sent ==
// Function 76, Protocol 4 - TCP, Subfunction 1, Read # Packets rcvd ==
// Function 76, Protocol 5 - ARP, Subfunction 0, Read # Packets sent ==
// Function 76, Protocol 5 - ARP, Subfunction 1, Read # Packets rcvd ==
static inline f76ret_t
umka_sys_net_arp_get_count(uint32_t dev_num) {
    f76ret_t r;
    __asm__ __inline__ __volatile__ (
        "call   i40"
        : "=a"(r.eax),
          "=b"(r.ebx)
        : "a"(76),
          "b"((5 << 16) + (dev_num << 8) + 2)
        : "memory");
    return r;
}

static inline f76ret_t
umka_sys_net_arp_get_entry(uint32_t dev_num, uint32_t arp_num, void *buf) {
    f76ret_t r;
    __asm__ __inline__ __volatile__ (
        "call   i40"
        : "=a"(r.eax),
          "=b"(r.ebx)
        : "a"(76),
          "b"((5 << 16) + (dev_num << 8) + 3),
          "c"(arp_num),
          "D"(buf)
        : "memory");
    return r;
}

static inline f76ret_t
umka_sys_net_arp_add_entry(uint32_t dev_num, void *buf) {
    f76ret_t r;
    __asm__ __inline__ __volatile__ (
        "call   i40"
        : "=a"(r.eax),
          "=b"(r.ebx)
        : "a"(76),
          "b"((5 << 16) + (dev_num << 8) + 4),
          "S"(buf)
        : "memory");
    return r;
}

static inline f76ret_t
umka_sys_net_arp_del_entry(uint32_t dev_num, int32_t arp_num) {
    f76ret_t r;
    __asm__ __inline__ __volatile__ (
        "call   i40"
        : "=a"(r.eax),
          "=b"(r.ebx)
        : "a"(76),
          "b"((5 << 16) + (dev_num << 8) + 5),
          "c"(arp_num)
        : "memory");
    return r;
}

// Function 76, Protocol 5 - ARP, Subfunction 6, Send ARP announce ==
// Function 76, Protocol 5 - ARP, Subfunction 7, Read # conflicts ===

static inline void
umka_set_keyboard_data(uint32_t scancode) {
    __asm__ __inline__ __volatile__ (
        "call   kos_set_keyboard_data"
        :
        : "c"(scancode)
        : "eax", "edx", "memory", "cc");
}

#endif  // UMKA_H_INCLUDED

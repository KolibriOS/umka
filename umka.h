#ifndef UMKA_H_INCLUDED
#define UMKA_H_INCLUDED

#include <inttypes.h>
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
    unsigned int (*adjust_cache_size)(void *userdata, size_t suggested_size) __attribute__((__stdcall__));
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
    uint8_t mac[6];
} net_device_t; // NET_DEVICE

typedef struct {
        void *next;     // pointer to next frame in list
        void *prev;     // pointer to previous frame in list
        net_device_t *device;   // ptr to NET_DEVICE structure
        uint32_t type;  // encapsulation type: e.g. Ethernet
        size_t length;  // size of encapsulated data
        size_t offset;  // offset to actual data (24 bytes for default frame)
        uint8_t data[0];
} net_buff_t;

typedef struct {
    uint32_t ip;
    uint8_t mac[6];
    uint16_t status;
    uint16_t ttl;
} arp_entry_t;


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

typedef struct {
    uint32_t value;
    uint32_t errorcode;
} f75ret_t;

typedef struct {
    uint32_t eax;
    uint32_t ebx;
} f76ret_t;

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

static inline void umka_i40(pushad_t *regs) {

    __asm__ __inline__ __volatile__ (
        "push   ebp;"
        "mov    ebp, %[ebp];"
        "call   i40;"
        "pop    ebp"
        : "=a"(regs->eax),
          "=b"(regs->ebx)
        : "a"(regs->eax),
          "b"(regs->ebx),
          "c"(regs->ecx),
          "d"(regs->edx),
          "S"(regs->esi),
          "D"(regs->edi),
          [ebp] "Rm"(regs->ebp)
        : "memory");
}

static inline void umka_sys_draw_window(size_t x, size_t xsize,
                                        size_t y, size_t ysize,
                                        uint32_t color, int has_caption,
                                        int client_relative,
                                        int fill_workarea, int gradient_fill,
                                        int movable, uint32_t style,
                                        const char *caption) {
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

static inline void umka_sys_set_pixel(size_t x, size_t y, uint32_t color,
                                      int invert) {
    __asm__ __inline__ __volatile__ (
        "call   i40"
        :
        : "a"(1),
          "b"(x),
          "c"(y),
          "d"((invert << 24) + color)
        : "memory");
}

static inline void umka_sys_write_text(size_t x, size_t y,
                                       uint32_t color,
                                       int asciiz, int fill_background,
                                       int font_and_encoding,
                                       int draw_to_buffer,
                                       int scale_factor,
                                       const char *string, size_t length,
                                       uintptr_t background_color_or_buffer) {
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

static inline void umka_sys_put_image(void *image, size_t xsize, size_t ysize,
                                      size_t x, size_t y) {
    __asm__ __inline__ __volatile__ (
        "call   i40"
        :
        : "a"(7),
          "b"(image),
          "c"((xsize << 16) + ysize),
          "d"((x << 16) + y)
        : "memory");
}

static inline void umka_sys_button(size_t x, size_t xsize,
                                   size_t y, size_t ysize,
                                   size_t button_id,
                                   int draw_button, int draw_frame,
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

static inline void umka_sys_process_info(int32_t pid, void *param) {
    __asm__ __inline__ __volatile__ (
        "call   i40"
        :
        : "a"(9),
          "b"(param),
          "c"(pid)
        : "memory");
}

static inline void umka_sys_window_redraw(int begin_end) {
    __asm__ __inline__ __volatile__ (
        "call   i40"
        :
        : "a"(12),
          "b"(begin_end)
        : "memory");
}

static inline void umka_sys_draw_rect(size_t x, size_t xsize,
                                      size_t y, size_t ysize,
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

static inline void umka_sys_get_screen_size(uint32_t *xsize, uint32_t *ysize) {
    uint32_t xysize;
    __asm__ __inline__ __volatile__ (
        "call   i40"
        : "=a"(xysize)
        : "a"(14)
        : "memory");
    *xsize = (xysize >> 16) + 1;
    *ysize = (xysize & 0xffffu) + 1;
}

static inline void umka_sys_set_cwd(const char *dir) {
    __asm__ __inline__ __volatile__ (
        "call   i40"
        :
        : "a"(30),
          "b"(1),
          "c"(dir)
        : "memory");
}

static inline void umka_sys_get_cwd(const char *buf, size_t len) {
    __asm__ __inline__ __volatile__ (
        "call   i40"
        :
        : "a"(30),
          "b"(2),
          "c"(buf),
          "d"(len)
        : "memory");
}

static inline void umka_sys_draw_line(size_t x, size_t xend,
                                      size_t y, size_t yend,
                                      uint32_t color, int invert) {
    __asm__ __inline__ __volatile__ (
        "call   i40"
        :
        : "a"(38),
          "b"((x << 16) + xend),
          "c"((y << 16) + yend),
          "d"((invert << 24) + color)
        : "memory");
}

static inline void umka_sys_display_number(int is_pointer, int base,
                                           int digits_to_display, int is_qword,
                                           int show_leading_zeros,
                                           int number_or_pointer,
                                           size_t x, size_t y,
                                           uint32_t color, int fill_background,
                                           int font, int draw_to_buffer,
                                           int scale_factor,
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

static inline void umka_sys_set_button_style(int style) {
    __asm__ __inline__ __volatile__ (
        "call   i40"
        :
        : "a"(48),
          "b"(1),
          "c"(style)
        : "memory");
}

static inline void umka_sys_set_window_colors(void *colors) {
    __asm__ __inline__ __volatile__ (
        "call   i40"
        :
        : "a"(48),
          "b"(2),
          "c"(colors),
          "d"(40)
        : "memory");
}

static inline void umka_sys_get_window_colors(void *colors) {
    __asm__ __inline__ __volatile__ (
        "call   i40"
        :
        : "a"(48),
          "b"(3),
          "c"(colors),
          "d"(40)
        : "memory");
}

static inline uint32_t umka_sys_get_skin_height() {
    uint32_t skin_height;
    __asm__ __inline__ __volatile__ (
        "call   i40"
        : "=a"(skin_height)
        : "a"(48),
          "b"(4)
        : "memory");
    return skin_height;
}

static inline void umka_sys_get_screen_area(rect_t *wa) {
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

static inline void umka_sys_set_screen_area(rect_t *wa) {
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

static inline void umka_sys_get_skin_margins(rect_t *wa) {
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

static inline int32_t umka_sys_set_skin(const char *path) {
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

static inline int umka_sys_get_font_smoothing() {
    int type;
    __asm__ __inline__ __volatile__ (
        "call   i40"
        : "=a"(type)
        : "a"(48),
          "b"(9)
        : "memory");
    return type;
}

static inline void umka_sys_set_font_smoothing(int type) {
    __asm__ __inline__ __volatile__ (
        "call   i40"
        :
        : "a"(48),
          "b"(10),
          "c"(type)
        : "memory");
}

static inline int umka_sys_get_font_size() {
    uint32_t size;
    __asm__ __inline__ __volatile__ (
        "call   i40"
        : "=a"(size)
        : "a"(48),
          "b"(11)
        : "memory");
    return size;
}

static inline void umka_sys_set_font_size(uint32_t size) {
    __asm__ __inline__ __volatile__ (
        "call   i40"
        :
        : "a"(48),
          "b"(12),
          "c"(size)
        : "memory");
}

static inline void umka_sys_put_image_palette(void *image,
                                              size_t xsize, size_t ysize,
                                              size_t x, size_t y,
                                              size_t bpp, void *palette,
                                              size_t row_offset) {
    __asm__ __inline__ __volatile__ (
        "push   ebp;"
        "mov    ebp, %[row_offset];"
        "call   i40;"
        "pop    ebp"
        :
        : "a"(65),
          "b"(image),
          "c"((xsize << 16) + ysize),
          "d"((x << 16) + y),
          "S"(bpp),
          "D"(palette),
          [row_offset] "Rm"(row_offset)
        : "memory");
}

static inline void umka_sys_move_window(size_t x, size_t y,
                                        ssize_t xsize, ssize_t ysize) {
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

static inline void umka_sys_lfn(void *f7080sXarg, f7080ret_t *r,
                                f70or80_t f70or80) {
    __asm__ __inline__ __volatile__ (
        "call   i40"
        : "=a"(r->status),
          "=b" (r->count)
        : "a"(f70or80),
          "b"(f7080sXarg)
        : "memory");
}

static inline void umka_sys_set_window_caption(const char *caption,
                                               int encoding) {
    __asm__ __inline__ __volatile__ (
        "call   i40"
        :
        : "a"(71),
          "b"(encoding ? 2 : 1),
          "c"(caption),
          "d"(encoding)
        : "memory");
}

static inline void umka_sys_blit_bitmap(int operation, int background,
                                        int transparent, int client_relative,
                                        void *params) {
    __asm__ __inline__ __volatile__ (
        "call   i40"
        :
        : "a"(73),
          "b"((client_relative << 29) + (transparent << 5) + (background << 4)
              + operation),
          "c"(params)
        : "memory");
}

static inline uint32_t umka_sys_net_get_dev_count() {
    uint32_t count;
    __asm__ __inline__ __volatile__ (
        "call   i40"
        : "=a"(count)
        : "a"(74),
          "b"(255)
        : "memory");
    return count;
}

static inline int32_t umka_sys_net_get_dev_type(uint8_t dev_num) {
    int32_t type;
    __asm__ __inline__ __volatile__ (
        "call   i40"
        : "=a"(type)
        : "a"(74),
          "b"((dev_num << 8) + 0)
        : "memory");
    return type;
}

static inline int32_t umka_sys_net_get_dev_name(uint8_t dev_num, char *name) {
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

static inline int32_t umka_sys_net_dev_reset(uint8_t dev_num) {
    int32_t status;
    __asm__ __inline__ __volatile__ (
        "call   i40"
        : "=a"(status)
        : "a"(74),
          "b"((dev_num << 8) + 2)
        : "memory");
    return status;
}

static inline int32_t umka_sys_net_dev_stop(uint8_t dev_num) {
    int32_t status;
    __asm__ __inline__ __volatile__ (
        "call   i40"
        : "=a"(status)
        : "a"(74),
          "b"((dev_num << 8) + 3)
        : "memory");
    return status;
}

static inline intptr_t umka_sys_net_get_dev(uint8_t dev_num) {
    intptr_t dev;
    __asm__ __inline__ __volatile__ (
        "call   i40"
        : "=a"(dev)
        : "a"(74),
          "b"((dev_num << 8) + 4)
        : "memory");
    return dev;
}

static inline uint32_t umka_sys_net_get_packet_tx_count(uint8_t dev_num) {
    uint32_t count;
    __asm__ __inline__ __volatile__ (
        "call   i40"
        : "=a"(count)
        : "a"(74),
          "b"((dev_num << 8) + 6)
        : "memory");
    return count;
}

static inline uint32_t umka_sys_net_get_packet_rx_count(uint8_t dev_num) {
    uint32_t count;
    __asm__ __inline__ __volatile__ (
        "call   i40"
        : "=a"(count)
        : "a"(74),
          "b"((dev_num << 8) + 7)
        : "memory");
    return count;
}

static inline uint32_t umka_sys_net_get_byte_tx_count(uint8_t dev_num) {
    uint32_t count;
    __asm__ __inline__ __volatile__ (
        "call   i40"
        : "=a"(count)
        : "a"(74),
          "b"((dev_num << 8) + 8)
        : "memory");
    return count;
}

static inline uint32_t umka_sys_net_get_byte_rx_count(uint8_t dev_num) {
    uint32_t count;
    __asm__ __inline__ __volatile__ (
        "call   i40"
        : "=a"(count)
        : "a"(74),
          "b"((dev_num << 8) + 9)
        : "memory");
    return count;
}

static inline uint32_t umka_sys_net_get_link_status(uint8_t dev_num) {
    uint32_t status;
    __asm__ __inline__ __volatile__ (
        "call   i40"
        : "=a"(status)
        : "a"(74),
          "b"((dev_num << 8) + 10)
        : "memory");
    return status;
}

static inline f75ret_t umka_sys_net_open_socket(uint32_t domain, uint32_t type,
                                                uint32_t protocol) {
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

static inline f75ret_t umka_sys_net_close_socket(uint32_t fd) {
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

static inline f75ret_t umka_sys_net_bind(uint32_t fd, void *sockaddr,
                                         size_t sockaddr_len) {
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

static inline f75ret_t umka_sys_net_listen(uint32_t fd, uint32_t backlog) {
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

static inline f75ret_t umka_sys_net_connect(uint32_t fd, void *sockaddr,
                                            size_t sockaddr_len) {
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

static inline f75ret_t umka_sys_net_accept(uint32_t fd, void *sockaddr,
                                           size_t sockaddr_len) {
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

static inline f76ret_t umka_sys_net_eth_read_mac(uint32_t dev_num) {
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

static inline f76ret_t umka_sys_net_ipv4_get_addr(uint32_t dev_num) {
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

static inline f76ret_t umka_sys_net_ipv4_set_addr(uint32_t dev_num,
                                                  uint32_t addr) {
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

static inline f76ret_t umka_sys_net_ipv4_get_dns(uint32_t dev_num) {
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

static inline f76ret_t umka_sys_net_ipv4_set_dns(uint32_t dev_num,
                                                 uint32_t dns) {
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

static inline f76ret_t umka_sys_net_ipv4_get_subnet(uint32_t dev_num) {
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

static inline f76ret_t umka_sys_net_ipv4_set_subnet(uint32_t dev_num,
                                                    uint32_t subnet) {
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

static inline f76ret_t umka_sys_net_ipv4_get_gw(uint32_t dev_num) {
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

static inline f76ret_t umka_sys_net_ipv4_set_gw(uint32_t dev_num,
                                                uint32_t gw) {
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
static inline f76ret_t umka_sys_net_arp_get_count(uint32_t dev_num) {
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

static inline f76ret_t umka_sys_net_arp_get_entry(uint32_t dev_num,
                                                  uint32_t arp_num,
                                                  void *buf) {
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

static inline f76ret_t umka_sys_net_arp_add_entry(uint32_t dev_num,
                                                  void *buf) {
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


// Function 76, Protocol 5 - ARP, Subfunction 4, Add ARP entry ====
// Function 76, Protocol 5 - ARP, Subfunction 5, Remove ARP entry ====
// Function 76, Protocol 5 - ARP, Subfunction 6, Send ARP announce ==
// Function 76, Protocol 5 - ARP, Subfunction 7, Read # conflicts ===

#endif  // UMKA_H_INCLUDED

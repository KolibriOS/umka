#ifndef KOLIBRI_H_INCLUDED
#define KOLIBRI_H_INCLUDED

#include <stdint.h>
#include <stddef.h>

#define BDFE_LEN_CP866 304
#define BDFE_LEN_UNICODE 560

enum {
    DEFAULT,
    CP866,
    UTF16,
    UTF8,
};

typedef enum {
    F70 = 70,
    F80 = 80,
} f70or80_t;

typedef enum {
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
} f70status_t;

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

uint32_t kos_time_to_epoch(uint32_t *time);
void kos_init(void);
void kos_lfn(void *f7080sXarg, f7080ret_t *r, f70or80_t f70or80);
void *kos_disk_add(const char *file_name, const char *disk_name);
int kos_disk_del(const char *name);
uint32_t kos_getcwd(char *buf, uint32_t len);
void kos_cd(const char *buf);

void hash_oneshot(void *ctx, void *data, size_t len);

void set_eflags_tf(int x);
void coverage_begin(void);
void coverage_end(void);
uint32_t get_lwp_event_size(void);

#endif

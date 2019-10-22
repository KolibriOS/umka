#ifndef KOLIBRI_H_INCLUDED
#define KOLIBRI_H_INCLUDED

#include <stdint.h>
#include <stddef.h>

enum {
    DEFAULT,
    CP866,
    UTF16,
    UTF8,
};

typedef enum {
    F70_SUCCESS,
    F70_DISK_BASE,
    F70_UNSUPPORTED_FS,
    F70_UNKNOWN_FS,
    F70_PARTITION,
    F70_FILE_NOT_FOUND,
    F70_END_OF_FILE,
    F70_MEMORY_POINTER,
    F70_DISK_FULL,
    F70_FS_FAIL,
    F70_ACCESS_DENIED,
    F70_DEVICE,
    F70_OUT_OF_MEMORY,
} f70status;

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
    char name[264];
} bdfe_t;

typedef struct {
    uint32_t status;
    uint32_t count;
} f70ret_t;

typedef struct {
    uint32_t sf;
    uint64_t offset;
    uint32_t count;
    void *buf;
    uint8_t zero;
    const char *path;
} __attribute__((packed)) f70s0arg_t;

typedef struct {
    uint32_t sf;
    uint32_t offset;
    uint32_t encoding;
    uint32_t size;
    void *buf;
    uint8_t zero;
    const char *path;
} __attribute__((packed)) f70s1arg_t;

typedef struct {
    uint32_t version;
    uint32_t cnt;
    uint32_t total_cnt;
    uint32_t reserved[5];
    bdfe_t bdfes[0];
} f70s1info_t;

typedef struct {
    uint32_t sf;
    uint32_t reserved1;
    uint32_t flags;
    uint32_t reserved2;
    void *buf;
    uint8_t zero;
    const char *path;
} __attribute__((packed)) f70s5arg_t;

#define KF_READONLY 0x01
#define KF_HIDDEN   0x02
#define KF_SYSTEM   0x04
#define KF_LABEL    0x08
#define KF_FOLDER   0x10

#define HASH_SIZE 32
typedef struct {
    uint8_t hash[HASH_SIZE];
    uint8_t opaque[1024-HASH_SIZE];
} hash_context;

uint32_t kos_time_to_epoch(uint32_t *time);
void *kos_fuse_init(int fd, uint32_t sect_cnt, uint32_t sect_sz);
void kos_fuse_lfn(void *f70sXarg, f70ret_t *r);

void kos_init(void);
void *kos_disk_add(const char *file_name, const char *disk_name);
int kos_disk_del(const char *name);

//void hash_init(void *ctx);
//void hash_update(void *ctx, void *data, size_t len);
//void hash_final(void *ctx);
void hash_oneshot(void *ctx, void *data, size_t len);

void set_eflags_tf(int x);
void coverage_begin(void);
void coverage_end(void);
uint32_t get_lwp_event_size(void);

#endif
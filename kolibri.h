#ifndef KOLIBRI_H_INCLUDED
#define KOLIBRI_H_INCLUDED

#include <stdint.h>

enum encoding {
    DEFAULT,
    CP866,
    UTF16,
    UTF8,
};

enum f70status {
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
};

struct bdfe {
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
};

struct f70s0arg {
    uint32_t sf;
    uint32_t offset_lo;
    uint32_t offset_hi;
    uint32_t size;
    void *buf;
    uint8_t zero;
    const char *path;
} __attribute__((packed));

struct f70s1arg {
    uint32_t sf;
    uint32_t offset;
    uint32_t encoding;
    uint32_t size;
    void *buf;
    uint8_t zero;
    const char *path;
} __attribute__((packed));

struct f70s1ret {
    uint32_t version;
    uint32_t cnt;
    uint32_t total_cnt;
    uint32_t reserved[5];
    struct bdfe bdfes[0];
};

struct f70s5arg {
    uint32_t sf;
    uint32_t reserved1;
    uint32_t flags;
    uint32_t reserved2;
    void *buf;
    uint8_t zero;
    const char *path;
} __attribute__((packed));

#define KF_READONLY 0x01
#define KF_HIDDEN   0x02
#define KF_SYSTEM   0x04
#define KF_LABEL    0x08
#define KF_FOLDER   0x10

uint32_t kos_time_to_epoch(uint32_t *time);
void *kos_fuse_init(int fd, uint32_t sect_cnt, uint32_t sect_sz);
int kos_fuse_lfn(void *f70arg);

#endif

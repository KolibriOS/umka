#ifndef KOS_H_INCLUDED
#define KOS_H_INCLUDED

#include <stdint.h>
#include <stdbool.h>

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

#define KF_READONLY 0x01
#define KF_HIDDEN   0x02
#define KF_SYSTEM   0x04
#define KF_LABEL    0x08
#define KF_FOLDER   0x10

uint32_t kos_time_to_epoch(uint32_t *time);
void *kos_fuse_init(int fd);
uint8_t *kos_fuse_readdir(const char *path, off_t offset);
void *kos_fuse_getattr(const char *path);
long *kos_fuse_read(const char *path, char *buf, size_t size, off_t offset);

#endif

#ifndef KOS_H_INCLUDED
#define KOS_H_INCLUDED

#include <stdint.h>

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

void kos_fuse_init(int fd);
uint8_t *kos_fuse_readdir(const char *path, off_t offset);
void *kos_fuse_getattr(const char *path);

#endif

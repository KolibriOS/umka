#ifndef KOS_H_INCLUDED
#define KOS_H_INCLUDED

void kos_fuse_init(int fd);
char *kos_fuse_readdir(const char *path, off_t offset);

#endif

#define FUSE_USE_VERSION 31

#include <fuse.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <stddef.h>
#include <stdlib.h>
#include <assert.h>
#include "kocdecl.h"

static void bdfe_to_stat(struct bdfe *kf, struct stat *st) {
    if (kf->attr & KF_FOLDER) {
        st->st_mode = S_IFDIR | 0755;
        st->st_nlink = 2;
    } else {
        st->st_mode = S_IFREG | 0644;
        st->st_nlink = 1;
        st->st_size = kf->size;
    }
}

static void *hello_init(struct fuse_conn_info *conn,
                        struct fuse_config *cfg) {
        (void) conn;
        cfg->kernel_cache = 1;
        return NULL;
}

static int hello_getattr(const char *path, struct stat *stbuf,
                         struct fuse_file_info *fi) {
        (void) fi;
        int res = 0;

        struct bdfe *kf = kos_fuse_getattr(path);
        bdfe_to_stat(kf, stbuf);
//        res = -ENOENT;
        return res;
}

static int hello_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
                         off_t offset, struct fuse_file_info *fi,
                         enum fuse_readdir_flags flags) {
        (void) offset;
        (void) fi;
        (void) flags;

        void *header = kos_fuse_readdir(path, offset);

        uint32_t i = *(uint32_t*)(header + 4);
        struct bdfe *kf = header + 0x20;
        for(; i>0; i--) {
                filler(buf, kf->name, NULL, 0, 0);
                kf++;
        }

        return 0;
}

static int hello_open(const char *path, struct fuse_file_info *fi) {
//        if (strcmp(path+1, "blah") != 0)
//                return -ENOENT;
        (void) path;

        if ((fi->flags & O_ACCMODE) != O_RDONLY)
                return -EACCES;

        return 0;
}

static int hello_read(const char *path, char *buf, size_t size, off_t offset,
                      struct fuse_file_info *fi) {
        (void) fi;

        kos_fuse_read(path, buf, size, offset);
        return size;
}

static struct fuse_operations hello_oper = {
        .init           = hello_init,
        .getattr        = hello_getattr,
        .readdir        = hello_readdir,
        .open           = hello_open,
        .read           = hello_read,
};

int main(int argc, char *argv[]) {
        if (argc != 3) {
                printf("usage: kofuse dir img\n");
                exit(1);
        }
        int fd = open(argv[2], O_RDONLY);
        kos_fuse_init(fd);
        return fuse_main(argc-1, argv, &hello_oper, NULL);
}

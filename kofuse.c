#define FUSE_USE_VERSION 31

#include <fuse.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <stddef.h>
#include <stdlib.h>
#include <time.h>
#include <assert.h>
#include "kolibri.h"

#define DIRENTS_TO_READ 100

static void bdfe_to_stat(struct bdfe *kf, struct stat *st) {
    if (kf->attr & KF_FOLDER) {
        st->st_mode = S_IFDIR | 0755;
        st->st_nlink = 2;
    } else {
        st->st_mode = S_IFREG | 0644;
        st->st_nlink = 1;
        st->st_size = kf->size;
    }
    st->st_atim = (struct timespec){ .tv_sec = kos_time_to_epoch(&(kf->atime)) };
    st->st_mtim = (struct timespec){ .tv_sec = kos_time_to_epoch(&(kf->mtime)) };
    st->st_ctim = (struct timespec){ .tv_sec = kos_time_to_epoch(&(kf->ctime)) };
}

static void *kofuse_init(struct fuse_conn_info *conn,
                        struct fuse_config *cfg) {
        (void) conn;
        cfg->kernel_cache = 1;
        return NULL;
}

static int kofuse_getattr(const char *path, struct stat *stbuf,
                         struct fuse_file_info *fi) {
    (void) fi;
    int res = 0;


    struct bdfe file;
    struct f70s5arg f70 = {5, 0, 0, 0, &file, 0, path};
    kos_fuse_lfn(&f70);

    bdfe_to_stat(&file, stbuf);
//   res = -ENOENT;
    return res;
}

static int kofuse_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
                         off_t offset, struct fuse_file_info *fi,
                         enum fuse_readdir_flags flags) {
    (void) offset;      // TODO
    (void) fi;
    (void) flags;

    struct f70s1ret *dir = (struct f70s1ret*)malloc(sizeof(struct f70s1ret) + sizeof(struct bdfe) * DIRENTS_TO_READ);
    struct f70s1arg f70 = {1, 0, CP866, DIRENTS_TO_READ, dir, 0, path};
    kos_fuse_lfn(&f70);
    for (size_t i = 0; i < dir->cnt; i++) {
        filler(buf, dir->bdfes[i].name, NULL, 0, 0);
    }
    free(dir);
    return 0;
}

static int kofuse_open(const char *path, struct fuse_file_info *fi) {
//        if (strcmp(path+1, "blah") != 0)
//                return -ENOENT;
        (void) path;

        if ((fi->flags & O_ACCMODE) != O_RDONLY)
                return -EACCES;

        return 0;
}

static int kofuse_read(const char *path, char *buf, size_t size, off_t offset,
                      struct fuse_file_info *fi) {
    (void) fi;

    struct f70s5arg f70 = {0, offset, offset >> 32, size, buf, 0, path};
    kos_fuse_lfn(&f70);
    return size;
}

static struct fuse_operations kofuse_oper = {
        .init           = kofuse_init,
        .getattr        = kofuse_getattr,
        .readdir        = kofuse_readdir,
        .open           = kofuse_open,
        .read           = kofuse_read,
};

int main(int argc, char *argv[]) {
        if (argc != 3) {
                printf("usage: kofuse dir img\n");
                exit(1);
        }
        int fd = open(argv[2], O_RDONLY);
        struct stat st;
        fstat(fd, &st);
        kos_fuse_init(fd, st.st_size / 512);
        return fuse_main(argc-1, argv, &kofuse_oper, NULL);
}

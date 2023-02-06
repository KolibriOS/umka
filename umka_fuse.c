/*
    SPDX-License-Identifier: GPL-2.0-or-later

    UMKa - User-Mode KolibriOS developer tools
    umka_fuse - FUSE <-> KolibriOS FS calls converter

    Copyright (C) 2017-2023  Ivan Baravy <dunkaist@gmail.com>
*/

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
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include "umkaio.h"
#include "vdisk.h"

#define DIRENTS_TO_READ 100

struct umka_fuse_ctx {
    struct umka_ctx *umka;
    struct umka_io *io;
};

static struct umka_fuse_ctx *
umka_fuse_init(void) {
    struct umka_fuse_ctx *ctx = malloc(sizeof(struct umka_fuse_ctx));
    ctx->umka = umka_init(UMKA_RUNNING_NEVER);
    ctx->io = io_init(&ctx->umka->running);
    return ctx;
}

static void
umka_fuse_close(struct umka_fuse_ctx *ctx) {
    umka_close(ctx->umka);
    io_close(ctx->io);
    free(ctx);
}

static void
bdfe_to_stat(bdfe_t *kf, struct stat *st) {
//    if (kf->attr & KF_FOLDER) {
    if (st) {
        st->st_mode = S_IFDIR | 0755;
        st->st_nlink = 2;
    } else {
        st->st_mode = S_IFREG | 0644;
        st->st_nlink = 1;
        st->st_size = kf->size;
    }
    st->st_atime = kos_time_to_epoch(&(kf->atime));
    st->st_mtime = kos_time_to_epoch(&(kf->mtime));
    st->st_ctime = kos_time_to_epoch(&(kf->ctime));
}

static void *
fs_init(struct fuse_conn_info *conn, struct fuse_config *cfg) {
        (void) conn;
        cfg->kernel_cache = 1;
        return NULL;
}

static void
fs_destroy(void *private_data) {
    struct umka_fuse_ctx *ctx = private_data;
    umka_fuse_close(ctx);
}
static int
fs_getattr(const char *path, struct stat *stbuf, struct fuse_file_info *fi) {
    (void) fi;
    int res = 0;

    bdfe_t file;
    f7080s5arg_t fX0 = {.sf = 5,
                        .flags = 0,
                        .buf = &file,
                        .u = {.f80 = {.path_encoding = UTF8,
                                      .path = path
                                     }
                             }
                       };
    f7080ret_t r;
    umka_sys_lfn(&fX0, &r, F80);

    bdfe_to_stat(&file, stbuf);
//   res = -ENOENT;
    return res;
}

static int
fs_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset,
             struct fuse_file_info *fi, enum fuse_readdir_flags flags) {
    (void) offset;      // TODO
    (void) fi;
    (void) flags;

    f7080s1info_t *dir = (f7080s1info_t*)malloc(sizeof(f7080s1info_t) +
                         BDFE_LEN_UNICODE * DIRENTS_TO_READ);
    f7080s1arg_t fX0 = {.sf = 1,
                        .offset = 0,
                        .encoding = UTF8,
                        .size = DIRENTS_TO_READ,
                        .buf = dir,
                        .u = {.f80 = {.path_encoding = UTF8,
                                      .path = path
                                     }
                             }
                       };
    f7080ret_t r;
    umka_sys_lfn(&fX0, &r, F80);
    bdfe_t *bdfe = dir->bdfes;
    for (size_t i = 0; i < dir->cnt; i++) {
        filler(buf, bdfe->name, NULL, 0, 0);
        bdfe = (bdfe_t*)((uintptr_t)bdfe + BDFE_LEN_UNICODE);
    }
    free(dir);
    return 0;
}

static int
fs_open(const char *path, struct fuse_file_info *fi) {
//    if (strcmp(path+1, "blah") != 0)
//        return -ENOENT;
    (void) path;

    if ((fi->flags & O_ACCMODE) != O_RDONLY)
        return -EACCES;

    return 0;
}

static int
fs_read(const char *path, char *buf, size_t size, off_t offset,
          struct fuse_file_info *fi) {
    (void) fi;

    f7080s0arg_t fX0 = {.sf = 0, .offset = offset, .count = size, .buf = buf,
                        .u = {.f80 = {.path_encoding = UTF8, .path = path}}};
    f7080ret_t r;
    umka_sys_lfn(&fX0, &r, F80);
    return size;
}

static struct fuse_operations umka_oper = {
    .init    = fs_init,
    .destroy = fs_destroy,
    .getattr = fs_getattr,
    .readdir = fs_readdir,
    .open    = fs_open,
    .read    = fs_read,
};

int
main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("usage: umka_fuse dir img\n");
        exit(1);
    }

    kos_boot.bpp = UMKA_DEFAULT_DISPLAY_BPP;
    kos_boot.x_res = UMKA_DEFAULT_DISPLAY_WIDTH;
    kos_boot.y_res = UMKA_DEFAULT_DISPLAY_HEIGHT;
    kos_boot.pitch = UMKA_DEFAULT_DISPLAY_WIDTH * UMKA_DEFAULT_DISPLAY_BPP / 8;

    struct umka_fuse_ctx *ctx = umka_fuse_init();

    struct vdisk *umka_disk = vdisk_init(argv[2], 1, 0u, 0);
    disk_t *disk = disk_add(&umka_disk->diskfunc, "hd0", umka_disk, 0);
    disk_media_changed(disk, 1);
    return fuse_main(argc-1, argv, &umka_oper, ctx);
}

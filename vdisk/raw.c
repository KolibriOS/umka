/*
    SPDX-License-Identifier: GPL-2.0-or-later

    UMKa - User-Mode KolibriOS developer tools
    vdisk - virtual disk, raw format

    Copyright (C) 2023  Ivan Baravy <dunkaist@gmail.com>
*/

#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include "../trace.h"
#include "umkaio.h"
#include "raw.h"

struct vdisk_raw {
    struct vdisk vdisk;
    int fd;
};

STDCALL void
vdisk_raw_close(void *userdata) {
    COVERAGE_OFF();
    struct vdisk_raw *disk = userdata;
    close(disk->fd);
    free(disk);
    COVERAGE_ON();
}

STDCALL int
vdisk_raw_read(void *userdata, void *buffer, off_t startsector,
               size_t *numsectors) {
    COVERAGE_OFF();
    struct vdisk_raw *disk = userdata;
    lseek(disk->fd, startsector * disk->vdisk.sect_size, SEEK_SET);
    io_read(disk->fd, buffer, *numsectors * disk->vdisk.sect_size,
            disk->vdisk.io);
    COVERAGE_ON();
    return KOS_ERROR_SUCCESS;
}

STDCALL int
vdisk_raw_write(void *userdata, void *buffer, off_t startsector,
                size_t *numsectors) {
    COVERAGE_OFF();
    struct vdisk_raw *disk = userdata;
    lseek(disk->fd, startsector * disk->vdisk.sect_size, SEEK_SET);
    io_write(disk->fd, buffer, *numsectors * disk->vdisk.sect_size,
             disk->vdisk.io);
    COVERAGE_ON();
    return KOS_ERROR_SUCCESS;
}

struct vdisk*
vdisk_init_raw(const char *fname, struct umka_io *io) {
    int fd = open(fname, O_RDONLY);
    if (!fd) {
        printf("[vdisk.raw]: can't open file '%s': %s\n", fname, strerror(errno));
        return NULL;
    }
    off_t fsize = lseek(fd, 0, SEEK_END);
    lseek(fd, 0, SEEK_SET);
    size_t sect_size = 512;
    if (strstr(fname, "s4096") != NULL || strstr(fname, "s4k") != NULL) {
        sect_size = 4096;
    }
    struct vdisk_raw *disk = (struct vdisk_raw*)malloc(sizeof(struct vdisk_raw));
    *disk = (struct vdisk_raw){
            .vdisk = {.diskfunc = {.strucsize = sizeof(diskfunc_t),
                                   .close = vdisk_raw_close,
                                   .read = vdisk_raw_read,
                                   .write = vdisk_raw_write,
                                  },
                      .sect_size = sect_size,
                      .sect_cnt = (uint64_t)fsize / sect_size,
                      .io = io,
                     },
            .fd = fd,
            };
    return (struct vdisk*)disk;
}

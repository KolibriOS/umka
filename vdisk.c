/*
    SPDX-License-Identifier: GPL-2.0-or-later

    UMKa - User-Mode KolibriOS developer tools
    vdisk - virtual disk

    Copyright (C) 2020-2023  Ivan Baravy <dunkaist@gmail.com>
    Copyright (C) 2021  Magomed Kostoev <mkostoevr@yandex.ru>
*/

#include <errno.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "umka.h"
#include "trace.h"
#include "vdisk.h"
#include "vdisk/raw.h"
#include "vdisk/qcow2.h"

STDCALL int
vdisk_querymedia(void *userdata, diskmediainfo_t *minfo) {
    COVERAGE_OFF();
    struct vdisk *disk = userdata;
    minfo->flags = 0u;
    minfo->sector_size = disk->sect_size;
    minfo->capacity = disk->sect_cnt;
    COVERAGE_ON();
    return KOS_ERROR_SUCCESS;
}

STDCALL size_t
vdisk_adjust_cache_size(void *userdata, size_t suggested_size) {
    struct vdisk *disk = userdata;
    if (disk->adjust_cache_size) {
        return disk->cache_size;
    } else {
        return suggested_size;
    }
}

struct vdisk*
vdisk_init(const char *fname, int adjust_cache_size, size_t cache_size,
           void *io) {
    size_t fname_len = strlen(fname);
    size_t dot_raw_len = strlen(RAW_SUFFIX);
    size_t dot_qcow2_len = strlen(QCOW2_SUFFIX);
    struct vdisk *disk;
    if (fname_len > dot_raw_len
        && !strcmp(fname + fname_len - dot_raw_len, RAW_SUFFIX)) {
        disk = (struct vdisk*)vdisk_init_raw(fname, io);
    } else if (fname_len > dot_qcow2_len
               && !strcmp(fname + fname_len - dot_qcow2_len, QCOW2_SUFFIX)) {
        disk = (struct vdisk*)vdisk_init_qcow2(fname, io);
    } else {
        fprintf(stderr, "[vdisk] file has unknown format: %s\n", fname);
        return NULL;
    }
    disk->diskfunc.closemedia = NULL;
    disk->diskfunc.querymedia = vdisk_querymedia;
    disk->diskfunc.flush = NULL;
    disk->diskfunc.adjust_cache_size = vdisk_adjust_cache_size;
    disk->adjust_cache_size = adjust_cache_size;
    disk->cache_size = cache_size;
    disk->io = io;
    return disk;
}

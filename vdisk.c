/*
    SPDX-License-Identifier: GPL-2.0-or-later

    UMKa - User-Mode KolibriOS developer tools
    vdisk - virtual disk

    Copyright (C) 2020-2022  Ivan Baravy <dunkaist@gmail.com>
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

#ifdef _WIN32
#define fseeko _fseeki64
#define ftello _ftelli64
#endif

STDCALL int
vdisk_querymedia(void *userdata, diskmediainfo_t *minfo) {
    COVERAGE_OFF();
    struct vdisk *disk = userdata;
    minfo->flags = 0u;
    minfo->sector_size = disk->sect_size;
    minfo->capacity = disk->sect_cnt;
    COVERAGE_ON();
    return ERROR_SUCCESS;
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
vdisk_init(const char *fname, int adjust_cache_size, size_t cache_size) {
    struct vdisk *disk;
    if (strstr(fname, ".img")) {
        disk = (struct vdisk*)vdisk_init_raw(fname);
    } else if (strstr(fname, ".qcow2")) {
        disk = (struct vdisk*)vdisk_init_qcow2(fname);
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
    return disk;
}

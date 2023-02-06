/*
    SPDX-License-Identifier: GPL-2.0-or-later

    UMKa - User-Mode KolibriOS developer tools
    vdisk - virtual disk

    Copyright (C) 2020-2023  Ivan Baravy <dunkaist@gmail.com>
*/

#ifndef VDISK_H_INCLUDED
#define VDISK_H_INCLUDED

#include <inttypes.h>
#include "umka.h"

struct vdisk {
    diskfunc_t diskfunc;
    uint32_t sect_size;
    uint64_t sect_cnt;
    unsigned cache_size;
    int adjust_cache_size;
    const void *io;
};

struct vdisk*
vdisk_init(const char *fname, const int adjust_cache_size,
           const size_t cache_size, const void *io);

#endif  // VDISK_H_INCLUDED

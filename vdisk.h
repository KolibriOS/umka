/*
    SPDX-License-Identifier: GPL-2.0-or-later

    UMKa - User-Mode KolibriOS developer tools
    vdisk - virtual disk

    Copyright (C) 2020-2022  Ivan Baravy <dunkaist@gmail.com>
*/

#ifndef VDISK_H_INCLUDED
#define VDISK_H_INCLUDED

#include <inttypes.h>
#include "umka.h"

void*
vdisk_init(const char *fname, int adjust_cache_size, size_t cache_size);

extern diskfunc_t vdisk_functions;

#endif  // VDISK_H_INCLUDED

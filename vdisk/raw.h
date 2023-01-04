/*
    SPDX-License-Identifier: GPL-2.0-or-later

    UMKa - User-Mode KolibriOS developer tools
    vdisk - virtual disk, raw format

    Copyright (C) 2023  Ivan Baravy <dunkaist@gmail.com>
*/

#ifndef VDISK_RAW_H_INCLUDED
#define VDISK_RAW_H_INCLUDED

#include <stdio.h>
#include "../vdisk.h"

struct vdisk_raw {
    struct vdisk vdisk;
    FILE *file;
};

struct vdisk_raw*
vdisk_init_raw(const char *fname);

#endif  // VDISK_RAW_H_INCLUDED

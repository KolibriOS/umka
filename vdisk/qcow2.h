/*
    SPDX-License-Identifier: GPL-2.0-or-later

    UMKa - User-Mode KolibriOS developer tools
    vdisk - virtual disk, qcow2 format

    Copyright (C) 2023  Ivan Baravy <dunkaist@gmail.com>
*/

#ifndef VDISK_QCOW2_H_INCLUDED
#define VDISK_QCOW2_H_INCLUDED

#include <stdio.h>
#include "../vdisk.h"

struct vdisk_qcow2 {
    struct vdisk vdisk;
    FILE *file;
};

struct vdisk_qcow2*
vdisk_init_qcow2(const char *fname);

#endif  // VDISK_QCOW2_H_INCLUDED

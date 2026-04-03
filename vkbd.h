/*
    SPDX-License-Identifier: GPL-2.0-or-later

    UMKa - User-Mode KolibriOS developer tools
    vkbd - virtual keyboard

    Copyright (C) 2025  Ivan Baravy <dunkaist@gmail.com>
*/

#ifndef VKBD_H_INCLUDED
#define VKBD_H_INCLUDED

#include <stdatomic.h>
#include "umka.h"

struct vkbd {
    int pew;
};

struct vkbd *
vkbd_init(void);

#endif  // VKBD_H_INCLUDED

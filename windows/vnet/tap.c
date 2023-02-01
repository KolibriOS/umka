/*
    SPDX-License-Identifier: GPL-2.0-or-later

    UMKa - User-Mode KolibriOS developer tools
    vnet - virtual network card, tap interface

    Copyright (C) 2023  Ivan Baravy <dunkaist@gmail.com>
*/

#include <stdio.h>

struct vnet *
vnet_init_tap() {
    fprintf(stderr, "[vnet] tap interface isn't implemented for windows\n");
    return NULL;
}

/*
    SPDX-License-Identifier: GPL-2.0-or-later

    UMKa - User-Mode KolibriOS developer tools
    vnet - virtual network card, tap interface

    Copyright (C) 2023  Ivan Baravy <dunkaist@gmail.com>
*/

#ifndef VNET_TAP_H_INCLUDED
#define VNET_TAP_H_INCLUDED

struct vnet *
vnet_init_tap(void);

#endif  // VNET_TAP_H_INCLUDED

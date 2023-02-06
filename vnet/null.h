/*
    SPDX-License-Identifier: GPL-2.0-or-later

    UMKa - User-Mode KolibriOS developer tools
    vnet - virtual network card, null interface

    Copyright (C) 2023  Ivan Baravy <dunkaist@gmail.com>
*/

#ifndef VNET_NULL_H_INCLUDED
#define VNET_NULL_H_INCLUDED

struct vnet *
vnet_init_null(void);

#endif  // VNET_NULL_H_INCLUDED

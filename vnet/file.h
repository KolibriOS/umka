/*
    SPDX-License-Identifier: GPL-2.0-or-later

    UMKa - User-Mode KolibriOS developer tools
    vnet - virtual network card, file interface

    Copyright (C) 2023  Ivan Baravy <dunkaist@gmail.com>
*/

#ifndef VNET_FILE_H_INCLUDED
#define VNET_FILE_H_INCLUDED

struct vnet *
vnet_init_file(void);

#endif  // VNET_FILE_H_INCLUDED

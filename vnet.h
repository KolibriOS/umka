/*
    SPDX-License-Identifier: GPL-2.0-or-later

    UMKa - User-Mode KolibriOS developer tools
    vnet - virtual network card

    Copyright (C) 2020-2022  Ivan Baravy <dunkaist@gmail.com>
*/

#ifndef VNET_H_INCLUDED
#define VNET_H_INCLUDED

#include "umka.h"

net_device_t*
vnet_init(void);

#endif  // VNET_H_INCLUDED

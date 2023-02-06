/*
    SPDX-License-Identifier: GPL-2.0-or-later

    UMKa - User-Mode KolibriOS developer tools
    vnet - virtual network card

    Copyright (C) 2020-2023  Ivan Baravy <dunkaist@gmail.com>
*/

#ifndef VNET_H_INCLUDED
#define VNET_H_INCLUDED

#include <stdatomic.h>
#include "umka.h"

#define VNET_BUFIN_CAP (NET_BUFFER_SIZE - offsetof(net_buff_t, data))

enum vnet_type {
    VNET_DEVTYPE_NULL,
    VNET_DEVTYPE_FILE,
    VNET_DEVTYPE_TAP,
};

struct vnet {
    struct eth_device eth;
    uint8_t bufin[VNET_BUFIN_CAP];
    size_t bufin_len;
    int fdin;
    int fdout;
    int input_processed;
    const atomic_int *running;
};

struct vnet *
vnet_init(enum vnet_type type, const atomic_int *running);

#endif  // VNET_H_INCLUDED

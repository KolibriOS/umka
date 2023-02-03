/*
    SPDX-License-Identifier: GPL-2.0-or-later

    UMKa - User-Mode KolibriOS developer tools
    vnet - virtual network card

    Copyright (C) 2020-2023  Ivan Baravy <dunkaist@gmail.com>
*/

#ifndef VNET_H_INCLUDED
#define VNET_H_INCLUDED

#include "umka.h"

#define UMKA_VNET_NAME "umka%d"
#define VNET_BUFIN_CAP (NET_BUFFER_SIZE - offsetof(net_buff_t, data))

enum vnet_type {
    VNET_FILE,
    VNET_TAP,
};

struct vnet {
    struct eth_device eth;
    uint8_t bufin[VNET_BUFIN_CAP];
    size_t bufin_len;
//    pthread_cond_t cond;
//    pthread_mutex_t mutex;
    int fdin;
    int fdout;
    int input_processed;
};

struct vnet *
vnet_init(enum vnet_type type);

#endif  // VNET_H_INCLUDED

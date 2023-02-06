/*
    SPDX-License-Identifier: GPL-2.0-or-later

    UMKa - User-Mode KolibriOS developer tools
    vnet - virtual network card, null interface

    Copyright (C) 2023  Ivan Baravy <dunkaist@gmail.com>
*/

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "trace.h"
#include "umka.h"
#include "vnet.h"

static STDCALL void
vnet_unload_null(void) {
    COVERAGE_ON();
    COVERAGE_OFF();
}

static STDCALL void
vnet_reset_null(void) {
    COVERAGE_ON();
    COVERAGE_OFF();
}

/*
static void
dump_net_buff(net_buff_t *buf) {
    for (size_t i = 0; i < buf->length; i++) {
        printf("%2.2x ", buf->data[i]);
    }
    putchar('\n');
}
*/

static STDCALL int
vnet_transmit_null(net_buff_t *buf) {
    struct vnet *net;
    __asm__ __inline__ __volatile__ (
        ""
        : "=b"(net)
        :
        : "memory");

//    dump_net_buff(buf);
    buf->length = 0;
    COVERAGE_OFF();
    COVERAGE_ON();
    return 0;
}

struct vnet *
vnet_init_null(void) {
    struct vnet *vnet = malloc(sizeof(struct vnet));
    vnet->eth.net.device_type = NET_TYPE_ETH;
    vnet->eth.net.mtu = 1514;
    char *devname = malloc(8);
    sprintf(devname, "UMKNUL%d", 0);   // FIXME: support more devices
    vnet->eth.net.name = devname;

    vnet->eth.net.unload = vnet_unload_null;
    vnet->eth.net.reset = vnet_reset_null;
    vnet->eth.net.transmit = vnet_transmit_null;

    vnet->fdin = 0;
    vnet->fdout = 0;
    vnet->input_processed = 1;

    memcpy(vnet->eth.mac, (uint8_t[]){0x80, 0x2b, 0xf9, 0x3b, 0x6c, 0xca},
           sizeof(vnet->eth.mac));

    return vnet;
}

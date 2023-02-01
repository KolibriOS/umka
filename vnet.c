/*
    SPDX-License-Identifier: GPL-2.0-or-later

    UMKa - User-Mode KolibriOS developer tools
    vnet - virtual network card

    Copyright (C) 2020-2023  Ivan Baravy <dunkaist@gmail.com>
    Copyright (C) 2021  Magomed Kostoev <mkostoevr@yandex.ru>
*/

#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#define _POSIX  // to have SIGSYS on windows
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "umka.h"
#include "trace.h"
#include "vnet.h"
#include "vnet/tap.h"
#include "vnet/file.h"

// TODO: Cleanup
#ifndef _WIN32
#include <arpa/inet.h>
#include <poll.h>
#include <unistd.h>
#else
#include <winsock2.h>
#include <pthread.h>
#endif

#define STACK_SIZE 0x10000

static int
vnet_input(void *udata) {
    umka_sti();
    struct vnet *net = udata;
    int fd = net->fdin;
    int plen = 0;
    fprintf(stderr, "[vnet] input interrupt\n");
    net_buff_t *buf = kos_net_buff_alloc(NET_BUFFER_SIZE);
    if (!buf) {
        fprintf(stderr, "[vnet] Can't allocate network buffer!\n");
        return 1;
    }
    buf->device = &net->netdev;
    plen = read(fd, buf->data, NET_BUFFER_SIZE - offsetof(net_buff_t, data));
    if (plen == -1) {
        plen = 0;   // we have just allocated a buffer, so we have to submit it
    }
    fprintf(stderr, "[vnet] read %i bytes\n", plen);
    for (int i = 0; i < plen; i++) {
        fprintf(stderr, " %2.2x", buf->data[i]);
    }
    fprintf(stderr, "\n");

    buf->length = plen;
    buf->offset = offsetof(net_buff_t, data);
    kos_eth_input(buf);
    net->input_processed = 1;

    return 1;   // acknowledge our interrupt
}

static void
vnet_input_monitor(struct vnet *net) {
    umka_sti();
    struct pollfd pfd = {net->fdin, POLLIN, 0};
    while (1) {
        if (net->input_processed && poll(&pfd, 1, 0)) {
            net->input_processed = 0;
            umka_irq_number = UMKA_IRQ_NETWORK;
            raise(SIGSYS);
            umka_sti();
        }
    }
}

struct vnet *
vnet_init(enum vnet_type type) {
//    printf("vnet_init\n");
    struct vnet *net;
    switch (type) {
    case VNET_FILE:
        net = vnet_init_file();
        break;
    case VNET_TAP:
        net = vnet_init_tap();
        break;
    default:
        fprintf(stderr, "[vnet] bad vnet type: %d\n", type);
        return NULL;
    }
    if (!net) {
        fprintf(stderr, "[vnet] device initialization failed\n");
        return NULL;
    }

    net->netdev.bytes_tx = 0;
    net->netdev.bytes_rx = 0;
    net->netdev.packets_tx = 0;
    net->netdev.packets_rx = 0;

    net->netdev.link_state = ETH_LINK_FD + ETH_LINK_10M;
    net->netdev.hwacc = 0;
    memcpy(net->netdev.mac, &(uint8_t[6]){0x80, 0x2b, 0xf9, 0x3b, 0x6c, 0xca},
           sizeof(net->netdev.mac));

    kos_attach_int_handler(UMKA_IRQ_NETWORK, vnet_input, net);
    fprintf(stderr, "[vnet] start input_monitor thread\n");
    uint8_t *stack = malloc(STACK_SIZE);
    size_t tid = umka_new_sys_threads(0, vnet_input_monitor, stack + STACK_SIZE);
    appdata_t *t = kos_slot_base + tid;
    *(void**)((uint8_t*)t->saved_esp0-12) = net;    // param for monitor thread
    // -12 here because in UMKa, unlike real hardware, we don't switch between
    // kernel and userspace, i.e. stack structure is different

    return net;
}

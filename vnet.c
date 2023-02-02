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
#include <stdatomic.h>
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
    struct vnet *vnet = udata;
    int fd = vnet->fdin;
    int plen = 0;
    fprintf(stderr, "[vnet] input interrupt\n");
    net_buff_t *buf = kos_net_buff_alloc(NET_BUFFER_SIZE);
    if (!buf) {
        fprintf(stderr, "[vnet] Can't allocate network buffer!\n");
        return 1;
    }
    buf->device = &vnet->eth.net;
    plen = read(fd, buf->data, NET_BUFFER_SIZE - offsetof(net_buff_t, data));
    if (plen == -1) {
        plen = 0;   // we have just allocated a buffer, so we have to submit it
    }
//if (plen != 0)
    fprintf(stderr, "[vnet] read %i bytes\n", plen);
    for (int i = 0; i < plen; i++) {
        fprintf(stderr, " %2.2x", buf->data[i]);
    }
    fprintf(stderr, "\n");

    buf->length = plen;
    buf->offset = offsetof(net_buff_t, data);
    kos_eth_input(buf);
    vnet->input_processed = 1;

    return 1;   // acknowledge our interrupt
}

static void
vnet_input_monitor(struct vnet *net) {
    umka_sti();
    struct pollfd pfd = {net->fdin, POLLIN, 0};
    while (1) {
        if (net->input_processed && poll(&pfd, 1, 0)) {
            net->input_processed = 0;
            atomic_store_explicit(&umka_irq_number, UMKA_IRQ_NETWORK,
                                  memory_order_release);
            raise(UMKA_SIGNAL_IRQ);
            umka_sti();
        }
    }
}

struct vnet *
vnet_init(enum vnet_type type) {
//    printf("vnet_init\n");
    struct vnet *vnet;
    switch (type) {
    case VNET_FILE:
        vnet = vnet_init_file();
        break;
    case VNET_TAP:
        vnet = vnet_init_tap();
        break;
    default:
        fprintf(stderr, "[vnet] bad vnet type: %d\n", type);
        return NULL;
    }
    if (!vnet) {
        fprintf(stderr, "[vnet] device initialization failed\n");
        return NULL;
    }

    vnet->eth.net.link_state = ETH_LINK_FD + ETH_LINK_10M;
    vnet->eth.net.hwacc = 0;

    vnet->eth.net.bytes_tx = 0;
    vnet->eth.net.bytes_rx = 0;

    vnet->eth.net.packets_tx = 0;
    vnet->eth.net.packets_tx_err = 0;
    vnet->eth.net.packets_tx_drop = 0;
    vnet->eth.net.packets_tx_ovr = 0;

    vnet->eth.net.packets_rx = 0;
    vnet->eth.net.packets_rx_err = 0;
    vnet->eth.net.packets_rx_drop = 0;
    vnet->eth.net.packets_rx_ovr = 0;

    kos_attach_int_handler(UMKA_IRQ_NETWORK, vnet_input, vnet);
    fprintf(stderr, "[vnet] start input_monitor thread\n");
    uint8_t *stack = malloc(STACK_SIZE);
    size_t tid = umka_new_sys_threads(0, vnet_input_monitor, stack + STACK_SIZE);
    appdata_t *t = kos_slot_base + tid;
    *(void**)((uint8_t*)t->saved_esp0-12) = vnet;    // param for monitor thread
    // -12 here because in UMKa, unlike real hardware, we don't switch between
    // kernel and userspace, i.e. stack structure is different

    return vnet;
}

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
#include <pthread.h>
#include <signal.h>
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "umka.h"
#include "umkart.h"
#include "trace.h"
#include "vnet.h"
#include "vnet/null.h"
#include "vnet/file.h"
#include "vnet/tap.h"

#ifndef _WIN32
#include <unistd.h>
#else
#include <io.h>
#endif

#define STACK_SIZE 0x10000

static int
vnet_input(void *udata) {
    umka_sti();
    int plen = 0;
    struct vnet *vnet = udata;
//    fprintf(stderr, "[vnet] input interrupt\n");
    net_buff_t *buf = kos_net_buff_alloc(NET_BUFFER_SIZE);
    if (!buf) {
        fprintf(stderr, "[vnet] Can't allocate network buffer!\n");
        return 1;
    }
    buf->device = &vnet->eth.net;
/*
    plen = read(fd, buf->data, NET_BUFFER_SIZE - offsetof(net_buff_t, data));
    if (plen == -1) {
        plen = 0;   // we have just allocated a buffer, so we have to submit it
    }
*/
    plen = vnet->bufin_len;
    memcpy(buf->data, vnet->bufin, vnet->bufin_len);
/*
    fprintf(stderr, "[vnet] read %i bytes\n", plen);
    for (int i = 0; i < plen; i++) {
        fprintf(stderr, " %2.2x", buf->data[i]);
    }
    fprintf(stderr, "\n");
*/
    buf->length = plen;
    buf->offset = offsetof(net_buff_t, data);
    kos_eth_input(buf);
    vnet->input_processed = 1;
//fprintf(stderr, "[vnet_input] signal before\n");
//    pthread_cond_signal(&vnet->cond);
//fprintf(stderr, "[vnet_input] signal after\n");

    return 1;   // acknowledge our interrupt
}

static void *
vnet_input_monitor(void *arg) {
    struct vnet *vnet = arg;
    while (1) {
        ssize_t nread = read(vnet->fdin, vnet->bufin, VNET_BUFIN_CAP);
        vnet->input_processed = 0;
        vnet->bufin_len = nread;
        atomic_store_explicit(&umka_irq_number, UMKA_IRQ_NETWORK,
                              memory_order_release);
        raise(UMKA_SIGNAL_IRQ); // FIXME: not atomic with the above
//fprintf(stderr, "[vnet_input_monitor] wait for signal\n");
//        pthread_cond_wait(&vnet->cond, &vnet->mutex);   // TODO: handle spurious
//fprintf(stderr, "[vnet_input_monitor] got signal\n");
    }
    return NULL;
}

struct vnet *
vnet_init(enum vnet_type type, const atomic_int *running) {
    struct vnet *vnet;
    switch (type) {
    case VNET_DEVTYPE_NULL:
        vnet = vnet_init_null();
        break;
    case VNET_DEVTYPE_FILE:
        vnet = vnet_init_file();
        break;
    case VNET_DEVTYPE_TAP:
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

    vnet->running = running;

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

//    pthread_cond_init(&vnet->cond, NULL);
//    pthread_mutex_init(&vnet->mutex, NULL);
//    pthread_mutex_lock(&vnet->mutex);

    kos_attach_int_handler(UMKA_IRQ_NETWORK, vnet_input, vnet);
    if (*running != UMKA_RUNNING_NEVER) {
        fprintf(stderr, "[vnet] start input_monitor thread\n");
        pthread_t thread_input_monitor;
        pthread_create(&thread_input_monitor, NULL, vnet_input_monitor, vnet);
    }

    return vnet;
}

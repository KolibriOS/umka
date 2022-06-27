/*
    SPDX-License-Identifier: GPL-2.0-or-later

    UMKa - User-Mode KolibriOS developer tools
    vnet - virtual network card

    Copyright (C) 2020-2022  Ivan Baravy <dunkaist@gmail.com>
    Copyright (C) 2021  Magomed Kostoev <mkostoevr@yandex.ru>
*/

#include <arpa/inet.h>
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <linux/if.h>
#include <linux/if_tun.h>
#include <net/if_arp.h>
#include <netinet/in.h>
#include <poll.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include "umka.h"
#include "trace.h"
#include "vnet.h"

// TODO: Cleanup
#ifndef _WIN32
#include <unistd.h>
#endif

#define TAP_DEV "/dev/net/tun"
#define UMKA_TAP_NAME "umka%d"

#define STACK_SIZE 0x10000

typedef struct {
    int tapfd;
    bool input_processed;
} vnet_userdata_t;

static int
tap_alloc() {
    struct ifreq ifr = {.ifr_name = UMKA_TAP_NAME,
                        .ifr_flags = IFF_TAP | IFF_NO_PI};
    int fd, err;

    if( (fd = open(TAP_DEV, O_RDWR | O_NONBLOCK)) < 0 ) {
        perror("Opening /dev/net/tun");
        return fd;
    }

    if( (err = ioctl(fd, TUNSETIFF, &ifr)) < 0 ) {
        perror("ioctl(TUNSETIFF)");
        close(fd);
        return err;
    }

    ifr.ifr_hwaddr.sa_family = ARPHRD_ETHER;
    memcpy(ifr.ifr_hwaddr.sa_data, (char[]){0x00, 0x11, 0x00, 0x00, 0x00, 0x00}, 6);

    if( (err = ioctl(fd, SIOCSIFHWADDR, &ifr)) < 0 ) {
        perror("ioctl(SIOCSIFHWADDR)");
        close(fd);
        return err;
    }

    struct sockaddr_in sai;
    sai.sin_family = AF_INET;
    sai.sin_port = 0;
    sai.sin_addr.s_addr = inet_addr("10.50.0.1");
    memcpy(&ifr.ifr_addr, &sai, sizeof(struct sockaddr));

    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);

    if ( (err = ioctl(sockfd, SIOCSIFADDR, &ifr)) < 0 ) {
        perror("ioctl(SIOCSIFADDR)");
        close(fd);
        return err;
    }

    sai.sin_addr.s_addr = inet_addr("255.255.255.0");
    memcpy(&ifr.ifr_netmask, &sai, sizeof(struct sockaddr));
    if ((err = ioctl(sockfd, SIOCSIFNETMASK, &ifr)) < 0) {
        perror("ioctl(SIOCSIFNETMASK)");
        close(fd);
        return err;
    }

    if ((err = ioctl(sockfd, SIOCGIFFLAGS, &ifr)) < 0) {
        perror("ioctl(SIOCGIFFLAGS)");
        close(fd);
        return err;
    }
    ifr.ifr_flags |= IFF_UP | IFF_RUNNING;
    ifr.ifr_flags &= ~(IFF_BROADCAST | IFF_LOWER_UP);
    if ((err = ioctl(sockfd, SIOCSIFFLAGS, &ifr)) < 0) {
        perror("ioctl(SIOCSIFFLAGS)");
        close(fd);
        return err;
    }

    return fd;
}

static int
vnet_input(void *udata) {
    umka_sti();
    net_device_t *vnet = udata;
    vnet_userdata_t *u = vnet->userdata;
    int tapfd = u->tapfd;
    uint8_t buffer[2048];
    int plen = 0;
    fprintf(stderr, "###### vnet_input\n");
    plen = read(tapfd, buffer, 2*1024);
    if (plen > 0) {
        fprintf(stderr, "[net_drv] read %i bytes\n", plen);
        for (int i = 0; i < plen; i++) {
            fprintf(stderr, " %2.2x", buffer[i]);
        }
        fprintf(stderr, "\n");

        net_buff_t *buf = kos_net_buff_alloc(plen + offsetof(net_buff_t, data));
        if (!buf) {
            fprintf(stderr, "[vnet] Can't allocate network buffer!\n");
            return 1;
        }
        buf->length = plen;
        buf->device = vnet;
        buf->offset = offsetof(net_buff_t, data);
        memcpy(buf->data, buffer, plen);
        kos_eth_input(buf);
    }
    u->input_processed = true;

    return 1;
}

static void
vnet_input_monitor(net_device_t *vnet) {
    umka_sti();
    vnet_userdata_t *u = vnet->userdata;
    int tapfd = u->tapfd;
    struct pollfd pfd = {tapfd, POLLIN, 0};
    while(true)
    {
        if (u->input_processed && poll(&pfd, 1, 0)) {
            u->input_processed = false;
            raise(SIGUSR1);
            umka_sti();
        }
    }
}

static void
dump_net_buff(net_buff_t *buf) {
    for (size_t i = 0; i < buf->length; i++) {
        printf("%2.2x ", buf->data[i]);
    }
    putchar('\n');
}

static STDCALL void
vnet_unload() {
    printf("vnet_unload\n");
    COVERAGE_ON();
    COVERAGE_OFF();
}

static STDCALL void
vnet_reset() {
    printf("vnet_reset\n");
    COVERAGE_ON();
    COVERAGE_OFF();
}

static STDCALL int
vnet_transmit(net_buff_t *buf) {
    // TODO: Separate implementations for win32 and linux
#ifdef _WIN32
    assert(!"Function is not implemented for win32");
#else
    net_device_t *vnet;
    __asm__ __inline__ __volatile__ (
        "nop"
        : "=b"(vnet)
        :
        : "memory");

    vnet_userdata_t *u = vnet->userdata;
    printf("vnet_transmit: %d bytes\n", buf->length);
    dump_net_buff(buf);
    write(u->tapfd, buf->data, buf->length);
    buf->length = 0;
    COVERAGE_OFF();
    COVERAGE_ON();
    printf("vnet_transmit: done\n");
#endif
    return 0;
}

net_device_t*
vnet_init() {
//    printf("vnet_init\n");
    int tapfd = tap_alloc();
    vnet_userdata_t *u = (vnet_userdata_t*)malloc(sizeof(vnet_userdata_t));
    u->tapfd = tapfd;
    u->input_processed = true;

    net_device_t *vnet = (net_device_t*)malloc(sizeof(net_device_t));
    *vnet = (net_device_t){
             .device_type = NET_TYPE_ETH,
             .mtu = 1514,
             .name = "UMK0770",

             .unload = vnet_unload,
             .reset = vnet_reset,
             .transmit = vnet_transmit,

             .bytes_tx = 0,
             .bytes_rx = 0,
             .packets_tx = 0,
             .packets_rx = 0,

             .link_state = ETH_LINK_FD + ETH_LINK_10M,
             .hwacc = 0,
             .mac = {0x80, 0x2b, 0xf9, 0x3b, 0x6c, 0xca},

             .userdata = u,
    };

    kos_attach_int_handler(SIGUSR1, vnet_input, vnet);
    fprintf(stderr, "### thread_start: %p\n", (void*)(uintptr_t)vnet_input_monitor);
    uint8_t *stack = malloc(STACK_SIZE);
    size_t tid = umka_new_sys_threads(0, vnet_input_monitor, stack + STACK_SIZE);
    appdata_t *t = kos_slot_base + tid;
    *(void**)((uint8_t*)t->pl0_stack+0x2000-12) = vnet;
//    t->saved_esp0 = (uint8_t*)t->saved_esp0 - 8;

    return vnet;
}

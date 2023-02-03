/*
    SPDX-License-Identifier: GPL-2.0-or-later

    UMKa - User-Mode KolibriOS developer tools
    vnet - virtual network card, tap interface

    Copyright (C) 2023  Ivan Baravy <dunkaist@gmail.com>
*/

#include <stdio.h>
#include <stdlib.h>

#include <fcntl.h>
#include <arpa/inet.h>
#include <linux/if.h>
#include <linux/if_tun.h>
#include <net/if_arp.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include "trace.h"
#include "vnet.h"

#define TAP_DEV "/dev/net/tun"

static STDCALL void
vnet_unload_tap() {
    printf("vnet_unload_tap\n");
    COVERAGE_ON();
    COVERAGE_OFF();
}

static STDCALL void
vnet_reset_tap() {
    printf("vnet_reset_tap\n");
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
vnet_transmit_tap(net_buff_t *buf) {
    struct vnet *net;
    __asm__ __inline__ __volatile__ (
        ""
        : "=b"(net)
        :
        : "memory");

//    printf("vnet_transmit: %d bytes\n", buf->length);
//    dump_net_buff(buf);
    ssize_t written = write(net->fdout, buf->data, buf->length);
    (void)written;
    buf->length = 0;
    COVERAGE_OFF();
    COVERAGE_ON();
//    printf("vnet_transmit: %d bytes written\n", written);
    return 0;
}

struct vnet *
vnet_init_tap() {
    struct ifreq ifr = {.ifr_name = UMKA_VNET_NAME,
                        .ifr_flags = IFF_TAP | IFF_NO_PI};
    int fd, err;

    if( (fd = open(TAP_DEV, O_RDWR)) < 0 ) {
        perror("Opening " TAP_DEV );
        return NULL;
    }

    if( (err = ioctl(fd, TUNSETIFF, &ifr)) < 0 ) {
        perror("ioctl(TUNSETIFF)");
        close(fd);
        return NULL;
    }

    ifr.ifr_hwaddr.sa_family = ARPHRD_ETHER;
    memcpy(ifr.ifr_hwaddr.sa_data, (char[]){0x00, 0x11, 0x00, 0x00, 0x00, 0x00}, 6);

    if( (err = ioctl(fd, SIOCSIFHWADDR, &ifr)) < 0 ) {
        perror("ioctl(SIOCSIFHWADDR)");
        close(fd);
        return NULL;
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
        return NULL;
    }

    sai.sin_addr.s_addr = inet_addr("255.255.255.0");
    memcpy(&ifr.ifr_netmask, &sai, sizeof(struct sockaddr));
    if ((err = ioctl(sockfd, SIOCSIFNETMASK, &ifr)) < 0) {
        perror("ioctl(SIOCSIFNETMASK)");
        close(fd);
        return NULL;
    }

    if ((err = ioctl(sockfd, SIOCGIFFLAGS, &ifr)) < 0) {
        perror("ioctl(SIOCGIFFLAGS)");
        close(fd);
        return NULL;
    }
    ifr.ifr_flags |= IFF_UP | IFF_RUNNING;
    ifr.ifr_flags &= ~(IFF_BROADCAST | IFF_LOWER_UP);
    if ((err = ioctl(sockfd, SIOCSIFFLAGS, &ifr)) < 0) {
        perror("ioctl(SIOCSIFFLAGS)");
        close(fd);
        return NULL;
    }

    struct vnet *vnet = malloc(sizeof(struct vnet));
    vnet->eth.net.device_type = NET_TYPE_ETH;
    vnet->eth.net.mtu = 1514;
    char *devname = malloc(8);
    sprintf(devname, "UMKTAP%d", 0);   // FIXME: support more devices
    vnet->eth.net.name = devname;

    vnet->eth.net.unload = vnet_unload_tap;
    vnet->eth.net.reset = vnet_reset_tap;
    vnet->eth.net.transmit = vnet_transmit_tap;

    vnet->fdin = fd;
    vnet->fdout = fd;
    vnet->input_processed = 1;

    memcpy(vnet->eth.mac, (uint8_t[]){0x80, 0x2b, 0xf9, 0x3b, 0x6c, 0xca},
           sizeof(vnet->eth.mac));

    return vnet;
}

/*
    umka_shell: User-Mode KolibriOS developer tools, the ping
    Copyright (C) 2020  Ivan Baravy <dunkaist@gmail.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <limits.h>
#include <linux/if.h>
#include <linux/if_tun.h>
#include <net/if_arp.h>
#include <netinet/in.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include "shell.h"
#include "umka.h"
#include "vnet.h"

#define TAP_DEV "/dev/net/tun"
#define UMKA_TAP_NAME "umka%d"

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

void
umka_thread_net_drv(void) {
    umka_sti();
    fprintf(stderr, "[net_drv] starting\n");
    int tapfd;
    uint8_t buffer[2048];
    int plen = 0;
    tapfd = tap_alloc();
    net_device_t *vnet = vnet_init(tapfd);
    kos_net_add_device(vnet);

    char devname[64];
    for (size_t i = 0; i < umka_sys_net_get_dev_count(); i++) {
        umka_sys_net_dev_reset(i);
        umka_sys_net_get_dev_name(i, devname);
        uint32_t devtype = umka_sys_net_get_dev_type(i);
        fprintf(stderr, "[net_drv] device %i: %s %u\n", i, devname, devtype);
    }

    f76ret_t r76;
    r76 = umka_sys_net_ipv4_set_subnet(1, inet_addr("255.255.255.0"));
    if (r76.eax == (uint32_t)-1) {
        fprintf(stderr, "[net_drv] set subnet error\n");
        return;
    }

//    r76 = umka_sys_net_ipv4_set_gw(1, inet_addr("192.168.1.1"));
    r76 = umka_sys_net_ipv4_set_gw(1, inet_addr("10.50.0.1"));
    if (r76.eax == (uint32_t)-1) {
        fprintf(stderr, "set gw error\n");
        return;
    }

    r76 = umka_sys_net_ipv4_set_dns(1, inet_addr("217.10.36.5"));
    if (r76.eax == (uint32_t)-1) {
        fprintf(stderr, "[net_drv] set dns error\n");
        return;
    }

    r76 = umka_sys_net_ipv4_set_addr(1, inet_addr("10.50.0.2"));
    if (r76.eax == (uint32_t)-1) {
        fprintf(stderr, "[net_drv] set ip addr error\n");
        return;
    }

    while(true)
    {
        plen = read(tapfd, buffer, 2*1024);
        if (plen > 0) {
            fprintf(stderr, "[net_drv] read %i bytes\n", plen);
            for (int i = 0; i < plen; i++) {
                fprintf(stderr, " %2.2x", buffer[i]);
            }
            fprintf(stderr, "\n");
            vnet_receive_frame(vnet, buffer, plen);
        } else if(plen == -1 && (errno == EAGAIN || errno == EINTR)) {
            continue;
        } else {
            perror("[net_drv] reading data");
            exit(1);
        }
    }

}

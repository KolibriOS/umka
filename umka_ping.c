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

#include <fcntl.h>
#include <inttypes.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <netinet/in.h>
#include "shell.h"
#include "umka.h"
#include "trace.h"
#include "vnet.h"

// tap
#include <stdint.h>
#include <net/if.h>
#include <linux/if_tun.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <arpa/inet.h> 
#include <sys/select.h>
#include <sys/time.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

static net_device_t vnet = {
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
                        };

uint8_t packet[4096] = {8,  0,  0, 0,  0, 0,  0, 0,  'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z', '0', '1', '2', '3', '4', '5'};

int tap_alloc(char *dev)
{
    int flags = IFF_TAP | IFF_NO_PI;
    struct ifreq ifr;
    int fd, err;
    char *clonedev = "/dev/net/tun";

    if( (fd = open(clonedev , O_RDWR)) < 0 )
    {
        perror("Opening /dev/net/tun");
        return fd;
    }

    memset(&ifr, 0, sizeof(ifr));

    ifr.ifr_flags = flags;

    if(*dev)
    {
        strncpy(ifr.ifr_name, dev, IFNAMSIZ);
    }

    if( (err = ioctl(fd, TUNSETIFF, (void *)&ifr)) < 0 )
    {
        perror("ioctl(TUNSETIFF)");
        close(fd);
        return err;
    }

    strcpy(dev, ifr.ifr_name);

    return fd;
}

int main(int argc, char **argv) {
    umka_tool = UMKA_SHELL;
    const char *usage = \
        "usage: umka_shell <ip | hostname> [-c]\n"
        "  -c               collect coverage";
    int opt;
    while ((opt = getopt(argc, argv, "c")) != -1) {
        switch (opt) {
        case 'c':
            coverage = 1;
            break;
        default:
            puts(usage);
            return 1;
        }
    }

    if (coverage)
        trace_begin();

    COVERAGE_ON();
    kos_init();
    COVERAGE_OFF();

    kos_stack_init();
    char tapdev[IFNAMSIZ] = "tap0";
    int tapfd = tap_alloc(tapdev);
    vnet_init(tapfd);

    kos_net_add_device(&vnet);
    char devname[64];
    umka_sys_net_dev_reset(1);
    for (size_t i = 0; i < umka_sys_net_get_dev_count(); i++) {
//        umka_sys_net_dev_reset(i);
        umka_sys_net_get_dev_name(i, devname);
        uint32_t devtype = umka_sys_net_get_dev_type(i);
        printf("device %i: %s %u\n", i, devname, devtype);
    }

    f76ret_t r76;
    r76 = umka_sys_net_ipv4_set_subnet(1, inet_addr("0.0.0.0"));
    if (r76.eax == (uint32_t)-1) {
        fprintf(stderr, "error\n");
        exit(1);
    }

    r76 = umka_sys_net_ipv4_set_gw(1, inet_addr("192.168.1.1"));
    if (r76.eax == (uint32_t)-1) {
        fprintf(stderr, "error\n");
        exit(1);
    }

    r76 = umka_sys_net_ipv4_set_dns(1, inet_addr("217.10.36.5"));
    if (r76.eax == (uint32_t)-1) {
        fprintf(stderr, "error\n");
        exit(1);
    }

    r76 = umka_sys_net_ipv4_set_addr(1, inet_addr("192.168.1.27"));
    if (r76.eax == (uint32_t)-1) {
        fprintf(stderr, "error\n");
        exit(1);
    }

    f75ret_t r75;
    r75 = umka_sys_net_open_socket(AF_INET4, SOCK_RAW, IPPROTO_ICMP);
    if (r75.errorcode == (uint32_t)-1) {
        fprintf(stderr, "error\n");
        exit(1);
    }
    uint32_t sockfd = r75.value;

//    uint32_t addr = inet_addr("127.0.0.1");
//    uint32_t addr = inet_addr("192.243.108.5");
    uint32_t addr = inet_addr("10.50.0.1");
    uint16_t port = 0;

    struct sockaddr_in sa;
    memset(&sa, 0, sizeof(sa));
    sa.sin_family = AF_INET4;
    sa.sin_port = htons(port);
    sa.sin_addr.s_addr = addr;

    r75 = umka_sys_net_connect(sockfd, &sa, sizeof(struct sockaddr_in));
    if (r75.errorcode == (uint32_t)-1) {
        fprintf(stderr, "error %u\n", r75.errorcode);
        exit(1);
    }

    r75 = umka_sys_net_send(sockfd, packet, 64, 0);
    if (r75.errorcode == (uint32_t)-1) {
        fprintf(stderr, "error %u\n", r75.errorcode);
        exit(1);
    }

    if (coverage)
        trace_end();

    return 0;
}

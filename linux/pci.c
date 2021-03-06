/*
    SPDX-License-Identifier: GPL-2.0-or-later

    UMKa - User-Mode KolibriOS developer tools

    Copyright (C) 2020-2021  Ivan Baravy <dunkaist@gmail.com>
*/

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <inttypes.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <limits.h>
#include "pci.h"

char pci_path[PATH_MAX] = ".";

__attribute__((stdcall)) uint32_t pci_read(uint32_t bus, uint32_t dev,
                                           uint32_t fun, uint32_t offset,
                                           size_t len) {
    char path_colon[PATH_MAX*2];
    char path_underscore[PATH_MAX*2];
    uint32_t value = 0;
    sprintf(path_colon, "%s/%4.4x:%2.2x:%2.2x.%u/config", pci_path, 0, bus, dev,
            fun);
    sprintf(path_underscore, "%s/%4.4x_%2.2x_%2.2x.%u/config", pci_path, 0, bus,
            dev, fun);
    int fd = open(path_colon, O_RDONLY);
    if (fd == -1) {
        fd = open(path_underscore, O_RDONLY);
        if (fd == -1) {
//            fprintf(stderr, "[pci] error: failed to open config file '%s': %s\n",
//                    path, strerror(errno));
            return UINT32_MAX;
        }
    }
    lseek(fd, offset, SEEK_SET);
    read(fd, &value, len);
    close(fd);
    return value;
}

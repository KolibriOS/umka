/*
    SPDX-License-Identifier: GPL-2.0-or-later

    UMKa - User-Mode KolibriOS developer tools
    io_async - input/output platform specific code

    Copyright (C) 2023  Ivan Baravy <dunkaist@gmail.com>
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <inttypes.h>
#include "umka.h"

#define QUEUE_DEPTH 1

#define read_barrier()  __asm__ __volatile__("":::"memory")
#define write_barrier() __asm__ __volatile__("":::"memory")

#define MAX(x, y) ((x) >= (y) ? (x) : (y))

void *
io_async_init() {
    fprintf(stderr, "[io_async] not implemented for windows\n");
    return NULL;
}

void
io_async_close(void *arg) {
    (void)arg;
}

ssize_t
io_async_read(int fd, void *buf, size_t count, void *arg) {
    (void)fd;
    (void)buf;
    (void)count;
    (void)arg;
    return -1;
}

ssize_t
io_async_write(int fd, const void *buf, size_t count, void *arg) {
    (void)fd;
    (void)buf;
    (void)count;
    (void)arg;
    return -1;
}

/*
    SPDX-License-Identifier: GPL-2.0-or-later

    UMKa - User-Mode KolibriOS developer tools
    io - input/output platform specific code

    Copyright (C) 2023  Ivan Baravy <dunkaist@gmail.com>
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <inttypes.h>
#include "umkaio.h"
#include "io_async.h"

struct umka_io *
io_init(int *running) {
    struct umka_io *io = malloc(sizeof(struct umka_io));
    io->running = running;
    io->async = io_async_init();
    return io;
}

void
io_close(struct umka_io *io) {
    io_async_close(io->async);
    free(io);
}

ssize_t
io_read(int fd, void *buf, size_t count, struct umka_io *io) {
    ssize_t res;
    if (!*io->running) {
        res = read(fd, buf, count);
    } else {
        res = io_async_read(fd, buf, count, io->async);
    }
    return res;
}

ssize_t
io_write(int fd, const void *buf, size_t count, struct umka_io *io) {
    ssize_t res;
    if (!*io->running) {
        res = write(fd, buf, count);
    } else {
        res = io_async_write(fd, buf, count, io->async);
    }
    return res;
}

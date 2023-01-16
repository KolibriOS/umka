/*
    SPDX-License-Identifier: GPL-2.0-or-later

    UMKa - User-Mode KolibriOS developer tools
    io - input/output platform specific code

    Copyright (C) 2023  Ivan Baravy <dunkaist@gmail.com>
*/

#include <stdlib.h>
#include <unistd.h>
#include "io.h"

struct umka_io {
    int change_task;
    int uring_fd;
};

void *
io_init(int change_task) {
    struct umka_io *io = malloc(sizeof(struct umka_io));
    io->change_task = change_task;
    if (change_task) {
        io->uring_fd = 1;
    }
    return io;
}

void
io_close(void *userdata) {
    struct umka_io *io = (struct umka_io*)userdata;
    free(io);
}

ssize_t
io_read(int fd, void *buf, size_t count, int change_task) {
    (void)change_task;
    ssize_t res;
    res = read(fd, buf, count);
    return res;
}

ssize_t
io_write(int fd, const void *buf, size_t count, int change_task) {
    (void)change_task;
    ssize_t res;
    res = write(fd, buf, count);
    return res;
}

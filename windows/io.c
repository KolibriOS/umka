/*
    SPDX-License-Identifier: GPL-2.0-or-later

    UMKa - User-Mode KolibriOS developer tools
    io - input/output platform specific code

    Copyright (C) 2023  Ivan Baravy <dunkaist@gmail.com>
*/

#include <unistd.h>
#include "io.h"

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

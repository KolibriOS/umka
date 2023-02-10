/*
    SPDX-License-Identifier: GPL-2.0-or-later

    UMKa - User-Mode KolibriOS developer tools
    io - input/output platform specific code

    Copyright (C) 2023  Ivan Baravy <dunkaist@gmail.com>
*/

#ifndef UMKAIO_H_INCLUDED
#define UMKAIO_H_INCLUDED

#include <stdatomic.h>
#include <stddef.h>
#include <pthread.h>

#if !defined (O_BINARY)
#define O_BINARY 0  // for Windows
#endif

struct umka_io {
    const atomic_int *running;
    pthread_t iot;
};

struct umka_io *
io_init(atomic_int *running);

void
io_close(struct umka_io *io);

ssize_t
io_read(int fd, void *buf, size_t count, const struct umka_io *io);

ssize_t
io_write(int fd, const void *buf, size_t count, const struct umka_io *io);

#endif  // UMKAIO_H_INCLUDED

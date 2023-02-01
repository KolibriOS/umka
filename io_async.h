/*
    SPDX-License-Identifier: GPL-2.0-or-later

    UMKa - User-Mode KolibriOS developer tools
    io_async - input/output platform specific code

    Copyright (C) 2023  Ivan Baravy <dunkaist@gmail.com>
*/

#ifndef IO_ASYNC_H_INCLUDED
#define IO_ASYNC_H_INCLUDED

#include <stddef.h>

void *
io_async_init();

void
io_async_close(void *arg);

ssize_t
io_async_read(int fd, void *buf, size_t count, void *arg);

ssize_t
io_async_write(int fd, const void *buf, size_t count, void *arg);

#endif  // IO_ASYNC_H_INCLUDED

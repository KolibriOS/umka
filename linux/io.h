/*
    SPDX-License-Identifier: GPL-2.0-or-later

    UMKa - User-Mode KolibriOS developer tools
    io - input/output platform specific code

    Copyright (C) 2023  Ivan Baravy <dunkaist@gmail.com>
*/

#ifndef IO_H_INCLUDED
#define IO_H_INCLUDED

#include <unistd.h>

ssize_t
io_read(int fd, void *buf, size_t count, int change_task);

ssize_t
io_write(int fd, const void *buf, size_t count, int change_task);

#endif  // IO_H_INCLUDED

/*
    SPDX-License-Identifier: GPL-2.0-or-later

    UMKa - User-Mode KolibriOS developer tools
    io - input/output platform specific code

    Copyright (C) 2023  Ivan Baravy <dunkaist@gmail.com>
*/

#ifndef IO_H_INCLUDED
#define IO_H_INCLUDED

#include <unistd.h>

#define IO_DONT_CHANGE_TASK 0
#define IO_CHANGE_TASK 1

void *
io_init(int change_task);

void
io_close(void *arg);

ssize_t
io_read(int fd, void *buf, size_t count, void *arg);

ssize_t
io_write(int fd, const void *buf, size_t count, void *arg);

#endif  // IO_H_INCLUDED

/*
    SPDX-License-Identifier: GPL-2.0-or-later

    UMKa - User-Mode KolibriOS developer tools
    io - input/output platform specific code

    Copyright (C) 2023  Ivan Baravy <dunkaist@gmail.com>
*/

#ifndef UMKAIO_H_INCLUDED
#define UMKAIO_H_INCLUDED

#include <stddef.h>
#include <pthread.h>

struct umka_io {
    const int *running;
    pthread_t iot;
};

struct iot_cmd_read_arg {
    int fd;
    void *buf;
    size_t count;
};

struct iot_cmd_read_ret {
    ssize_t val;
};

union iot_cmd_read {
    struct iot_cmd_read_arg arg;
    struct iot_cmd_read_ret ret;
};

struct iot_cmd_write_arg {
    int fd;
    void *buf;
    size_t count;
};

struct iot_cmd_write_ret {
    ssize_t val;
};

union iot_cmd_write {
    struct iot_cmd_write_arg arg;
    struct iot_cmd_write_ret ret;
};

enum {
    IOT_CMD_TYPE_READ,
    IOT_CMD_TYPE_WRITE,
};

struct iot_cmd {
    pthread_cond_t iot_cond;
    pthread_mutex_t iot_mutex;
    pthread_mutex_t mutex;
    int status;
    int type;
    union {
        union iot_cmd_read read;
        union iot_cmd_write write;
    };
};

struct umka_io *
io_init(int *running);

void
io_close(struct umka_io *io);

ssize_t
io_read(int fd, void *buf, size_t count, struct umka_io *io);

ssize_t
io_write(int fd, const void *buf, size_t count, struct umka_io *io);

#endif  // UMKAIO_H_INCLUDED

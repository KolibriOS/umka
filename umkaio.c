/*
    SPDX-License-Identifier: GPL-2.0-or-later

    UMKa - User-Mode KolibriOS developer tools
    io - input/output platform specific code

    Copyright (C) 2023  Ivan Baravy <dunkaist@gmail.com>
*/

#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <inttypes.h>
#include "umka.h"
#include "umkaio.h"

#define IOT_QUEUE_DEPTH 1

struct iot_cmd iot_cmd_buf[IOT_QUEUE_DEPTH];

static void *
thread_io(void *arg) {
    (void)arg;
    for (size_t i = 0; i < IOT_QUEUE_DEPTH; i++) {
        iot_cmd_buf[i].status = UMKA_CMD_STATUS_EMPTY;
        iot_cmd_buf[i].type = 0;
        pthread_cond_init(&iot_cmd_buf[i].iot_cond, NULL);
        pthread_mutex_init(&iot_cmd_buf[i].iot_mutex, NULL);
        pthread_mutex_lock(&iot_cmd_buf[i].iot_mutex);
        pthread_mutex_init(&iot_cmd_buf[i].mutex, NULL);
    }

    struct iot_cmd *cmd = iot_cmd_buf;
    ssize_t ret;
    while (1) {
        pthread_cond_wait(&cmd->iot_cond, &cmd->iot_mutex);
        // status must be ready
        switch (cmd->type) {
        case IOT_CMD_TYPE_READ:
            ret = read(cmd->read.arg.fd, cmd->read.arg.buf, cmd->read.arg.count);
            cmd->read.ret.val = ret;
            break;
        case IOT_CMD_TYPE_WRITE:
            cmd->read.ret.val = write(cmd->read.arg.fd, cmd->read.arg.buf,
                                      cmd->read.arg.count);
            break;
        default:
            break;
        }

    atomic_store_explicit(&cmd->status, UMKA_CMD_STATUS_DONE, memory_order_release);
    }

    return NULL;
}

static uint32_t
io_async_submit_wait_test(void) {
//    appdata_t *app;
//    __asm__ __volatile__ ("":"=b"(app)::);
//    struct io_uring_queue *q = app->wait_param;
    int done = pthread_mutex_trylock(&iot_cmd_buf[0].mutex);
    return done;
}

static uint32_t
io_async_complete_wait_test(void) {
//    appdata_t *app;
//    __asm__ __volatile__ ("":"=b"(app)::);
//    struct io_uring_queue *q = app->wait_param;
    int status = atomic_load_explicit(&iot_cmd_buf[0].status, memory_order_acquire);
    return status == UMKA_CMD_STATUS_DONE;
}

ssize_t
io_async_read(int fd, void *buf, size_t count, void *arg) {
    (void)arg;

    kos_wait_events(io_async_submit_wait_test, NULL);
    // status must be empty
    struct iot_cmd *cmd = iot_cmd_buf;
    cmd->read.arg.fd = fd;
    cmd->read.arg.buf = buf;
    cmd->read.arg.count = count;
    atomic_store_explicit(&cmd->status, UMKA_CMD_STATUS_READY, memory_order_release);
    
    pthread_cond_signal(&cmd->iot_cond);
    kos_wait_events(io_async_complete_wait_test, NULL);

    ssize_t res = cmd->read.ret.val;

    atomic_store_explicit(&cmd->status, UMKA_CMD_STATUS_EMPTY, memory_order_release);
    pthread_mutex_unlock(&cmd->mutex);

    return res;
}

ssize_t
io_async_write(int fd, const void *buf, size_t count, void *arg) {
    (void)fd;
    (void)buf;
    (void)count;
    (void)arg;
    return -1;
}

struct umka_io *
io_init(atomic_int *running) {
    struct umka_io *io = malloc(sizeof(struct umka_io));
    io->running = running;
    if (running) {
        pthread_create(&io->iot, NULL, thread_io, NULL);
    }
    return io;
}

void
io_close(struct umka_io *io) {
    free(io);
}

ssize_t
io_read(int fd, void *buf, size_t count, struct umka_io *io) {
    ssize_t res;
    if (!io->running || !*io->running) {
        res = read(fd, buf, count);
    } else {
        res = io_async_read(fd, buf, count, NULL);
    }
    return res;
}

ssize_t
io_write(int fd, const void *buf, size_t count, struct umka_io *io) {
    ssize_t res;
    if (!io->running || !*io->running) {
        res = write(fd, buf, count);
    } else {
        res = io_async_write(fd, buf, count, NULL);
    }
    return res;
}

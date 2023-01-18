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
#include <sys/mman.h>
#include <sys/syscall.h>
#include <linux/io_uring.h>
#include "../umka.h"

#define QUEUE_DEPTH 1

#define read_barrier()  __asm__ __volatile__("":::"memory")
#define write_barrier() __asm__ __volatile__("":::"memory")

#define MAX(x, y) ((x) >= (y) ? (x) : (y))

struct io_uring_queue {
    int fd;
    void *base;
    size_t base_mmap_len;
    struct io_uring_params p;
    uint32_t *array;
    struct io_uring_sqe *sqes;
    size_t sqes_mmap_len;
    uint32_t *sq_head, *sq_tail;
    uint32_t sq_mask;
    struct io_uring_cqe *cqes;
    uint32_t *cq_head, *cq_tail;
    uint32_t cq_mask;
};

static int
io_uring_setup(unsigned entries, struct io_uring_params *p) {
    return (int) syscall(__NR_io_uring_setup, entries, p);
}

static int
io_uring_enter(int ring_fd, unsigned int to_submit, unsigned int min_complete,
               unsigned int flags) {
    return (int) syscall(__NR_io_uring_enter, ring_fd, to_submit, min_complete,
                   flags, NULL, 0);
}

/*
static void
dump_scht(char *prefix, struct io_uring_queue *q) {
fprintf(stderr, "# async %s: %p %p\n", prefix, (void*)q, q->base);
    size_t head, tail, mask;
    read_barrier();
    mask = q->sq_mask;
    mask = *(uint32_t*)(((uintptr_t)q->base) + q->p.sq_off.ring_mask);
    head = *q->sq_head & mask;
    tail = *q->sq_tail & mask;
fprintf(stderr, "######### %s ######### sq %u:%u 0x%x\n", prefix, head, tail, mask);
    mask = q->cq_mask;
    mask = *(uint32_t*)(((uintptr_t)q->base) + q->p.cq_off.ring_mask);
    head = *q->cq_head & mask;
    tail = *q->cq_tail & mask;
fprintf(stderr, "######### %s ######### cq %u:%u 0x%x\n", prefix, head, tail, mask);
}
*/

int
cq_has_data(struct io_uring_queue *q) {
    size_t head, tail;
    read_barrier();
    head = *q->cq_head & q->cq_mask;
    tail = *q->cq_tail & q->cq_mask;
    return head != tail;
}

static void
build_op_read(struct io_uring_sqe *sqe, int fd, void *data, size_t size,
              off_t offset) {
    sqe->fd = fd;
    sqe->flags = 0;
    sqe->opcode = IORING_OP_READ;
    sqe->addr = (uintptr_t)data;
    sqe->len = size;
    sqe->off = offset;
    sqe->user_data = 0;
}

static int
read_from_cq(struct io_uring_queue *q) {
    size_t head, tail;
    struct io_uring_cqe *cqe;
    do {
        read_barrier();
        head = *q->cq_head & q->cq_mask;
        tail = *q->cq_tail & q->cq_mask;
        if (head == tail)
            break;

        /* Get the entry */
        cqe = q->cqes + head;
        if (cqe->res < 0) {
            fprintf(stderr, "Read error: %s\n", strerror(abs(cqe->res)));
        }
        (*q->cq_head)++;
    } while (head == tail);
    write_barrier();
    return cqe->res;
}

struct io_uring_queue *
io_async_init() {
    struct io_uring_queue *q = calloc(1, sizeof(struct io_uring_queue));
    q->fd = io_uring_setup(QUEUE_DEPTH, &q->p);
    if (q->fd < 0) {
        perror("io_uring_setup");
        return NULL;
    }

    size_t sring_size = q->p.sq_off.array + q->p.sq_entries * sizeof(unsigned);
    size_t cring_size = q->p.cq_off.cqes
                      + q->p.cq_entries * sizeof(struct io_uring_cqe);
    size_t rings_size = MAX(sring_size, cring_size);
    q->base_mmap_len = rings_size;

    if (!(q->p.features & IORING_FEAT_SINGLE_MMAP)) {
        fprintf(stderr, "[io.uring] Your kernel doesn't support"
                " IORING_FEAT_SINGLE_MMAP, upgrade to Linux 5.4\n");
        return NULL;
    }

    q->base = mmap(0, rings_size, PROT_READ | PROT_WRITE, MAP_SHARED
                   | MAP_POPULATE, q->fd, IORING_OFF_SQ_RING);
    if (q->base == MAP_FAILED) {
        perror("[io.uring] Can't mmap io_uring rings");
        return NULL;
    }

    q->sqes_mmap_len = q->p.sq_entries * sizeof(struct io_uring_sqe);
    q->sqes = mmap(0, q->sqes_mmap_len, PROT_READ | PROT_WRITE, MAP_SHARED
                   | MAP_POPULATE, q->fd, IORING_OFF_SQES);
    if (q->sqes == MAP_FAILED) {
        perror("[io.uring] Can't mmap io_uring SQEs");
        return NULL;
    }

    q->cqes = (struct io_uring_cqe*)((uintptr_t)q->base + q->p.cq_off.cqes);
    q->array = (uint32_t*)((uintptr_t)q->base + q->p.sq_off.array);
    q->sq_head = (uint32_t*)((uintptr_t)q->base + q->p.sq_off.head);
    q->sq_tail = (uint32_t*)((uintptr_t)q->base + q->p.sq_off.tail);
    q->sq_mask = *(uint32_t*)((uintptr_t)q->base + q->p.sq_off.ring_mask);
    q->cq_head = (uint32_t*)((uintptr_t)q->base + q->p.cq_off.head);
    q->cq_tail = (uint32_t*)((uintptr_t)q->base + q->p.cq_off.tail);
    q->cq_mask = *(uint32_t*)((uintptr_t)q->base + q->p.cq_off.ring_mask);

    return q;
}

void
io_async_close(void *arg) {
    struct io_uring_queue *q = arg;
    munmap(q->base, q->base_mmap_len);
    munmap(q->sqes, q->sqes_mmap_len);
    close(q->fd);
    free(q);
}

static uint32_t
io_uring_wait_test() {
    appdata_t *app;
    __asm__ __volatile__ ("":"=b"(app)::);
    struct io_uring_queue *q = app->wait_param;
    int done = cq_has_data(q);
    return done;
}

ssize_t
io_async_read(int fd, void *buf, size_t count, void *arg) {
    struct io_uring_queue *q = arg;
    read_barrier();
    size_t tail = *q->sq_tail & q->sq_mask;
    struct io_uring_sqe *sqe = q->sqes + tail;
    build_op_read(sqe, fd, buf, count, -1);
    q->array[tail] = tail;
    (*q->sq_tail)++;

    int ret =  io_uring_enter(q->fd, 1, 0, IORING_ENTER_GETEVENTS);
    if(ret < 0) {
        perror("io_uring_enter");
        return 1;
    }

    kos_wait_events(io_uring_wait_test, q);

    ssize_t res = read_from_cq(q);

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

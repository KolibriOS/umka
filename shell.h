/*
    SPDX-License-Identifier: GPL-2.0-or-later

    UMKa - User-Mode KolibriOS developer tools
    umka_shell - the shell

    Copyright (C) 2020-2023  Ivan Baravy <dunkaist@gmail.com>
*/

#ifndef SHELL_H_INCLUDED
#define SHELL_H_INCLUDED

#include <stdatomic.h>
#include <stdio.h>
#include <pthread.h>
#include "umka.h"
#include "umkaio.h"
#include "optparse/optparse.h"

enum shell_var_type {
    SHELL_VAR_SINT,
    SHELL_VAR_UINT,
    SHELL_VAR_POINTER,
};

#define SHELL_LOG_NONREPRODUCIBLE 0
#define SHELL_LOG_REPRODUCIBLE 1

#define SHELL_VAR_NAME_LEN 16

struct shell_var {
    struct shell_var *next;
    enum shell_var_type type;
    char name[SHELL_VAR_NAME_LEN];
    union {ssize_t sint; size_t uint; uint64_t uint64; void *ptr;} value;
};

struct shell_ctx {
    const struct umka_ctx *umka;
    struct umka_io *io;
    int reproducible;
    const char *hist_file;
    struct shell_var *var;
    FILE *fin;
    FILE *fout;
    const atomic_int *running;
    pthread_cond_t cmd_done;
    pthread_mutex_t cmd_mutex;
    struct optparse opts;
};

struct shell_ctx *
shell_init(int reproducible, const char *hist_file, struct umka_ctx *umka,
           struct umka_io *io, FILE *fin, const atomic_int *running);

void
shell_close(struct shell_ctx *shell);

void *
run_test(struct shell_ctx *ctx);

void
umka_run_cmd_sync(struct shell_ctx *ctx);

#endif  // SHELL_H_INCLUDED

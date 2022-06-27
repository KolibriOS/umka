/*
    SPDX-License-Identifier: GPL-2.0-or-later

    UMKa - User-Mode KolibriOS developer tools
    umka_shell - the shell

    Copyright (C) 2020-2022  Ivan Baravy <dunkaist@gmail.com>
*/

#ifndef SHELL_H_INCLUDED
#define SHELL_H_INCLUDED

#include <stdio.h>

enum shell_var_type {
    SHELL_VAR_SINT,
    SHELL_VAR_UINT,
    SHELL_VAR_POINTER,
};

#define SHELL_VAR_NAME_LEN 16

struct shell_var {
    struct shell_var *next;
    enum shell_var_type type;
    char name[SHELL_VAR_NAME_LEN];
    union {ssize_t sint; size_t uint; uint64_t uint64; void *ptr;} value;
};

struct shell_ctx {
    int reproducible;
    const char *hist_file;
    struct shell_var *var;
};

void *run_test(struct shell_ctx *ctx);

#endif  // SHELL_H_INCLUDED

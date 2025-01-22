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
    const struct umka_io *io;
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
shell_init(const int reproducible, const char *hist_file,
           const struct umka_ctx *umka, const struct umka_io *io, FILE *fin);

void
shell_close(struct shell_ctx *shell);

void *
run_test(struct shell_ctx *ctx);

enum {
    UMKA_CMD_NONE,
    UMKA_CMD_SET_MOUSE_DATA,
    UMKA_CMD_WAIT_FOR_IDLE,
    UMKA_CMD_WAIT_FOR_OS_IDLE,
    UMKA_CMD_WAIT_FOR_WINDOW,
    UMKA_CMD_SYS_CSLEEP,
    UMKA_CMD_SYS_PROCESS_INFO,
    UMKA_CMD_SYS_GET_MOUSE_POS_SCREEN,
    UMKA_CMD_SYS_LFN,
    UMKA_CMD_SEND_SCANCODE,
};

struct cmd_set_mouse_data_arg {
    uint32_t btn_state;
    int32_t xmoving;
    int32_t ymoving;
    int32_t vscroll;
    int32_t hscroll;
};

struct cmd_set_mouse_data_ret {
    char stub;
};

struct cmd_set_mouse_data {
    struct cmd_set_mouse_data_arg arg;
    struct cmd_set_mouse_data_ret ret;
};

struct cmd_sys_lfn_arg {
    f70or80_t f70or80;
    f7080s1arg_t *bufptr;
    f7080ret_t *r;
};

struct cmd_sys_lfn_ret {
    f7080ret_t status;
};

struct cmd_sys_lfn {
    struct cmd_sys_lfn_arg arg;
    struct cmd_sys_lfn_ret ret;
};

struct cmd_sys_process_info_arg {
    int32_t pid;
    void *param;
};

struct cmd_sys_process_info_ret {
    char stub;
};

struct cmd_sys_process_info {
    struct cmd_sys_process_info_arg arg;
    struct cmd_sys_process_info_ret ret;
};

struct cmd_sys_get_mouse_pos_screen_arg {
    char stub;
};

struct cmd_sys_get_mouse_pos_screen_ret {
    struct point16s pos;
};

struct cmd_sys_get_mouse_pos_screen {
    struct cmd_sys_get_mouse_pos_screen_arg arg;
    struct cmd_sys_get_mouse_pos_screen_ret ret;
};

struct cmd_sys_csleep_arg {
    uint32_t csec;
};

struct cmd_sys_csleep_ret {
    char stub;
};

struct cmd_sys_csleep {
    struct cmd_sys_csleep_arg arg;
    struct cmd_sys_csleep_ret ret;
};

struct cmd_wait_for_window_arg {
    char *wnd_title;
};

struct cmd_wait_for_window_ret {
    char stub;
};

struct cmd_wait_for_window {
    struct cmd_wait_for_window_arg arg;
    struct cmd_wait_for_window_ret ret;
};

struct cmd_send_scancode_arg {
    int scancode;
};

struct cmd_send_scancode_ret {
    char stub;
};

struct cmd_send_scancode {
    struct cmd_send_scancode_arg arg;
    struct cmd_send_scancode_ret ret;
};

struct umka_cmd {
    atomic_int status;
    uint32_t type;
    union {
        // internal funcs
        struct cmd_set_mouse_data set_mouse_data;
        struct cmd_send_scancode send_scancode;
        struct cmd_wait_for_window wait_for_window;
        // syscalls
        struct cmd_sys_csleep sys_csleep;
        struct cmd_sys_process_info sys_process_info;
        struct cmd_sys_lfn sys_lfn;
        struct cmd_sys_get_mouse_pos_screen sys_get_mouse_pos_screen;
    };
};

struct umka_cmd *
shell_get_cmd(struct shell_ctx *shell);

void
shell_run_cmd(struct shell_ctx *ctx);

void
shell_clear_cmd(struct umka_cmd *cmd);

#endif  // SHELL_H_INCLUDED

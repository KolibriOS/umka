/*
    SPDX-License-Identifier: GPL-2.0-or-later

    UMKa - User-Mode KolibriOS developer tools
    monitor - the command interface

    Copyright (C) 2025  Ivan Baravy <dunkaist@gmail.com>
*/

#ifndef MONITOR_H_INCLUDED
#define MONITOR_H_INCLUDED

#include <pthread.h>
#include <stdatomic.h>
#include "umka.h"

struct monitor_ctx {
    const atomic_int *running;
    pthread_cond_t cmd_done;
    pthread_mutex_t cmd_mutex;
    FILE *fout;
};

enum {
    UMKA_CMD_NONE,
    UMKA_CMD_SET_MOUSE_DATA,
    UMKA_CMD_WAIT_FOR_IDLE,
    UMKA_CMD_WAIT_FOR_OS_IDLE,
    UMKA_CMD_WAIT_FOR_WINDOW,
    UMKA_CMD_SYS_CSLEEP,
    UMKA_CMD_SYS_PROCESS_INFO,
    UMKA_CMD_SYS_GET_MOUSE_POS_SCREEN,
    UMKA_CMD_SYS_SET_MOUSE_POS_SCREEN,
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
    enum f70or80 f70or80;
    union f7080arg *bufptr;
    struct f7080ret *r;
};

struct cmd_sys_lfn_ret {
    struct f7080ret status;
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

struct cmd_sys_set_mouse_pos_screen_arg {
    struct point16s pos;
};

struct cmd_sys_set_mouse_pos_screen_ret {
    char stub;
};

struct cmd_sys_set_mouse_pos_screen {
    struct cmd_sys_set_mouse_pos_screen_arg arg;
    struct cmd_sys_set_mouse_pos_screen_ret ret;
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
        struct cmd_sys_set_mouse_pos_screen sys_set_mouse_pos_screen;
    };
};

struct umka_cmd *
monitor_get_cmd(struct monitor_ctx *mon);

void
monitor_run_cmd(struct monitor_ctx *mon);

void
monitor_clear_cmd(struct umka_cmd *cmd);

struct monitor_ctx *
monitor_init(atomic_int *running);

void
monitor_cmd_send_scancodes(struct monitor_ctx *mon, uint8_t *scancodes);

void
monitor_cmd_boot(struct monitor_ctx *mon);

void
monitor_cmd_csleep(struct monitor_ctx *mon, uint32_t csec);

void
monitor_cmd_wait_for_idle(struct monitor_ctx *mon);

void
monitor_cmd_wait_for_os_idle(struct monitor_ctx *mon);

void
monitor_cmd_wait_for_window(struct monitor_ctx *mon, char *wnd_title);

void
monitor_cmd_mouse_move(struct monitor_ctx *mon, uint32_t btn_state,
                       int32_t xmoving, int32_t ymoving, int32_t vscroll,
                       int32_t hscroll);

struct f7080ret
monitor_cmd_sys_lfn(struct monitor_ctx *on, enum f70or80 f70or80,
                    union f7080arg *fX0);

void
monitor_cmd_sys_set_mouse_pos_screen(struct monitor_ctx *mon, int16_t x,
                                     int16_t y);

#endif  // MONITOR_H_INCLUDED

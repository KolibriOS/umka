/*
    SPDX-License-Identifier: GPL-2.0-or-later

    UMKa - User-Mode KolibriOS developer tools
    umka_os - kind of KolibriOS anykernel

    Copyright (C) 2023  Ivan Baravy <dunkaist@gmail.com>
*/

#include "umka.h"

enum {
    UMKA_CMD_NONE,
    UMKA_CMD_SET_MOUSE_DATA,
    UMKA_CMD_SYS_PROCESS_INFO,
    UMKA_CMD_SYS_GET_MOUSE_POS_SCREEN,
};

enum {
    UMKA_CMD_STATUS_EMPTY,
    UMKA_CMD_STATUS_READY,
    UMKA_CMD_STATUS_DONE,
};

struct cmd_set_mouse_data {
    uint32_t btn_state;
    int32_t xmoving;
    int32_t ymoving;
    int32_t vscroll;
    int32_t hscroll;
};

struct cmd_sys_process_info {
    int32_t pid;
    void *param;
};

struct cmd_ret_sys_get_mouse_pos_screen {
    struct point16s pos;
};

struct umka_cmd {
    uint32_t status;
    uint32_t type;
    union {
        struct cmd_set_mouse_data set_mouse_data;
    } arg;
    union {
        struct cmd_ret_sys_get_mouse_pos_screen sys_get_mouse_pos_screen;
    } ret;
};

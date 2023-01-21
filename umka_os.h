/*
    SPDX-License-Identifier: GPL-2.0-or-later

    UMKa - User-Mode KolibriOS developer tools
    umka_os - kind of KolibriOS anykernel

    Copyright (C) 2023  Ivan Baravy <dunkaist@gmail.com>
*/

struct shared_info {
    uint64_t pid;
    uint32_t lfb_base;
    uint32_t lfb_bpp;
    uint32_t lfb_width;
    uint32_t lfb_height;
    uint32_t cmd_buf;
    uint32_t pad;
};

#define CMD_BUF_LEN 0x10000

uint8_t cmd_buf[CMD_BUF_LEN];



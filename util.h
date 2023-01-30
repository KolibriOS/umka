/*
    SPDX-License-Identifier: GPL-2.0-or-later

    UMKa - User-Mode KolibriOS developer tools

    Copyright (C) 2021, 2023  Ivan Baravy <dunkaist@gmail.com>
*/

#ifndef UTIL_H_INCLUDED
#define UTIL_H_INCLUDED

void
dump_devices_dat(const char *filename);

void
copy_display_to_rgb888(void *to);

#endif  // UTIL_H_INCLUDED

/*
    SPDX-License-Identifier: GPL-2.0-or-later

    UMKa - User-Mode KolibriOS developer tools

    Copyright (C) 2021, 2023  Ivan Baravy <dunkaist@gmail.com>
*/

#ifndef UMKART_H_INCLUDED
#define UMKART_H_INCLUDED

#include <stdatomic.h>
#include "umka.h"
#include "shell.h"

extern atomic_int umka_irq_number;

void
dump_devices_dat(const char *filename);

void
copy_display_to_rgb888(void *to);

#endif  // UMKART_H_INCLUDED

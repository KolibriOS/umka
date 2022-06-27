/*
    SPDX-License-Identifier: GPL-2.0-or-later

    UMKa - User-Mode KolibriOS developer tools

    Copyright (C) 2019-2020  Ivan Baravy <dunkaist@gmail.com>
*/

#ifndef TRACE_LBR_H_INCLUDED
#define TRACE_LBR_H_INCLUDED

#include <inttypes.h>

void trace_lbr_begin(void);
void trace_lbr_end(void);
uint32_t trace_lbr_pause(void);
void trace_lbr_resume(uint32_t value);

#endif

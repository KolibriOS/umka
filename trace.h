/*
    SPDX-License-Identifier: GPL-2.0-or-later

    UMKa - User-Mode KolibriOS developer tools

    Copyright (C) 2019-2020  Ivan Baravy <dunkaist@gmail.com>
*/

#ifndef TRACE_H_INCLUDED
#define TRACE_H_INCLUDED

#include <inttypes.h>

extern uint32_t coverage;

#define COVERAGE_ON() do { trace_resume(coverage); } while (0)

#define COVERAGE_OFF() do { coverage = trace_pause(); } while (0)

void trace_begin(void);
void trace_end(void);
uint32_t trace_pause(void);
void trace_resume(uint32_t value);

#endif

/*
    SPDX-License-Identifier: GPL-2.0-or-later

    UMKa - User-Mode KolibriOS developer tools

    Copyright (C) 2019-2020  Ivan Baravy <dunkaist@gmail.com>
*/

#ifndef TRACE_H_INCLUDED
#define TRACE_H_INCLUDED

#define COVERAGE_ON() do { trace_on(); } while (0)

#define COVERAGE_OFF() do { trace_off(); } while (0)

void trace_enable(void);
void trace_disable(void);
void trace_on(void);
void trace_off(void);

#endif

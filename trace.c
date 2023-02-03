/*
    SPDX-License-Identifier: GPL-2.0-or-later

    UMKa - User-Mode KolibriOS developer tools

    Copyright (C) 2019-2020  Ivan Baravy <dunkaist@gmail.com>
*/

#include "trace_lbr.h"

uint32_t coverage;

void
trace_begin(void) {
    trace_lbr_begin();
}

void
trace_end(void) {
    trace_lbr_end();
}

uint32_t
trace_pause(void) {
    return trace_lbr_pause();
}

void
trace_resume(uint32_t value) {
    trace_lbr_resume(value);
}

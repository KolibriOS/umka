/*
    SPDX-License-Identifier: GPL-2.0-or-later

    UMKa - User-Mode KolibriOS developer tools

    Copyright (C) 2019-2020  Ivan Baravy <dunkaist@gmail.com>
*/

#include "trace_lbr.h"

void
trace_enable(void) {
    trace_lbr_enable();
}

void
trace_disable(void) {
    trace_lbr_disable();
}

void
trace_on(void) {
    trace_lbr_on();
}

void
trace_off(void) {
    trace_lbr_off();
}

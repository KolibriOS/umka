/*
    SPDX-License-Identifier: GPL-2.0-or-later

    UMKa - User-Mode KolibriOS developer tools

    Copyright (C) 2021  Magomed Kostoev <mkostoevr@yandex.ru>
*/

#include <stdio.h>
#include <stdlib.h>

void
reset_procmask(void) {
    printf("STUB: %s:%d", __FILE__, __LINE__);
}

int
get_fake_if(void *ctx) {
    (void)ctx;
    printf("STUB: %s:%d", __FILE__, __LINE__);
    return 0;
}

void
system_shutdown(void) {
    exit(0);
}

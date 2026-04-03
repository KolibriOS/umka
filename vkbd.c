/*
    SPDX-License-Identifier: GPL-2.0-or-later

    UMKa - User-Mode KolibriOS developer tools
    vkbd - virtual keyboard

    Copyright (C) 2025  Ivan Baravy <dunkaist@gmail.com>
*/

#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <pthread.h>
#include <signal.h>
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "umka.h"
#include "umkart.h"
#include "trace.h"
#include "vkbd.h"

struct vkbd *
vkbd_init(void) {
    struct vkbd *vkbd = (struct vkbd*)malloc(sizeof(struct vkbd));
    if (!vkbd) {
        fprintf(stderr, "[vkbd] device initialization failed\n");
        return NULL;
    }

    vkbd->pew = 0;

    return vkbd;
}

/*
    SPDX-License-Identifier: GPL-2.0-or-later

    UMKa - User-Mode KolibriOS developer tools

    Copyright (C) 2020,2022  Ivan Baravy <dunkaist@gmail.com>
*/

#include <setjmp.h>
#define __USE_GNU
#include <signal.h>
#include <stddef.h>
#include <stdlib.h>

sigset_t mask;

void
reset_procmask(void) {
    sigemptyset (&mask);
	sigaddset (&mask, SIGALRM);
    sigprocmask(SIG_UNBLOCK, &mask, NULL);
}

int
get_fake_if(ucontext_t *ctx) {
    // we fake IF with ID flag
    return !(ctx->uc_mcontext.__gregs[REG_EFL] & (1 << 21));
}

void
system_shutdown(void) {
    exit(0);
}

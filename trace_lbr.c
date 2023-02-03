/*
    SPDX-License-Identifier: GPL-2.0-or-later

    UMKa - User-Mode KolibriOS developer tools

    Copyright (C) 2019-2020,2022-2023  Ivan Baravy <dunkaist@gmail.com>
    Copyright (C) 2021  Magomed Kostoev <mkostoevr@yandex.ru>
*/

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>

#ifndef _WIN32
#include <unistd.h>
#include <fcntl.h>
#endif

#include "umka.h"

#define MSR_IA32_DEBUGCTL           0x1d9
#define MSR_IA32_DEBUGCTL_LBR       0x1    // enable profiling
#define MSR_IA32_DEBUGCTL_BTF       0x2    // profile only branches if EFLAGS.TF
#define MSR_IA32_LASTBRANCHFROMIP   0x1db
#define MSR_IA32_LASTBRANCHTOIP     0x1dc

int msrfd;

uint64_t rdmsr(uint32_t reg)
{
    uint64_t data = 0;

#ifndef _WIN32
    if (pread(msrfd, &data, sizeof data, reg) != sizeof data) {
        perror("rdmsr: pread");
        exit(1);
    }
#else
    (void)reg;
    printf("STUB: %s:%d", __FILE__, __LINE__);
#endif

    return data;
}

void wrmsr(uint32_t reg, uint64_t data)
{
#ifndef _WIN32
    int fd;
    fd = open("/dev/cpu/0/msr", O_WRONLY);
    if (fd < 0) {
        perror("wrmsr: open");
        exit(1);
    }

    if (pwrite(fd, &data, sizeof data, reg) != sizeof data) {
        perror("wrmsr: pwrite");
        exit(1);
    }

    close(fd);
#else
    (void)reg;
    (void)data;
    printf("STUB: %s:%d", __FILE__, __LINE__);
#endif
}

void handle_sigtrap(int signo) {
    (void)signo;
#ifndef _WIN32
    uint64_t from = rdmsr(MSR_IA32_LASTBRANCHFROMIP);
    uint64_t to = rdmsr(MSR_IA32_LASTBRANCHTOIP);

    if (from >= (uintptr_t)coverage_begin && from < (uintptr_t)coverage_end) {
        coverage_table[from - (uintptr_t)coverage_begin].from_cnt++;
    }

    if (to >= (uintptr_t)coverage_begin && to < (uintptr_t)coverage_end) {
        coverage_table[to - (uintptr_t)coverage_begin].to_cnt++;
    }

    wrmsr(MSR_IA32_DEBUGCTL, MSR_IA32_DEBUGCTL_LBR + MSR_IA32_DEBUGCTL_BTF);
#else
    printf("STUB: %s:%d", __FILE__, __LINE__);
#endif
}

uint32_t set_eflags_tf(uint32_t tf);

void trace_lbr_begin(void) {
#ifndef _WIN32
    struct sigaction action;
    action.sa_handler = &handle_sigtrap;
    action.sa_flags = 0;
    sigaction(SIGTRAP, &action, NULL);

    wrmsr(MSR_IA32_DEBUGCTL, MSR_IA32_DEBUGCTL_LBR + MSR_IA32_DEBUGCTL_BTF);
    msrfd = open("/dev/cpu/0/msr", O_RDONLY);
    if (msrfd < 0) {
        perror("rdmsr: open");
        exit(1);
    }
#else
    printf("STUB: %s:%d", __FILE__, __LINE__);
#endif
}

void trace_lbr_end(void) {
#ifndef _WIN32
    wrmsr(MSR_IA32_DEBUGCTL, 0);
    close(msrfd);
#else
    printf("STUB: %s:%d", __FILE__, __LINE__);
#endif
}

uint32_t trace_lbr_pause(void) {
    return set_eflags_tf(0u);
}

void trace_lbr_resume(uint32_t value) {
    set_eflags_tf(value);
}

#define _GNU_SOURCE
#include <stdint.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>
#include <sched.h>
#include "kolibri.h"

int covfd, msrfd;

uint64_t rdmsr(uint32_t reg)
{
    uint64_t data;

    if (pread(msrfd, &data, sizeof data, reg) != sizeof data) {
        perror("rdmsr: pread");
        exit(1);
    }

    return data;
}

void wrmsr(uint32_t reg, uint64_t data)
{
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
    return;
}

void handle_sigtrap() {
    uint64_t from = rdmsr(0x1db);
    uint64_t to = rdmsr(0x1dc);

    if ((from >= (uintptr_t)coverage_begin && from < (uintptr_t)coverage_end) ||
        (to >= (uintptr_t)coverage_begin && to < (uintptr_t)coverage_end)) {
        write(covfd, &from, 8);
        write(covfd, &to, 8);
    }

    wrmsr(0x1d9, 3);
}

void trace_lbr_begin() {
    printf("hello from lbr!\n");
    struct sigaction action;
    action.sa_sigaction = &handle_sigtrap;
    action.sa_flags = SA_SIGINFO;
    sigaction(SIGTRAP, &action, NULL);

    cpu_set_t my_set;
    CPU_ZERO(&my_set);
    CPU_SET(0, &my_set);
    sched_setaffinity(0, sizeof(cpu_set_t), &my_set);
    wrmsr(0x1d9, 3);
    msrfd = open("/dev/cpu/0/msr", O_RDONLY);
    if (msrfd < 0) {
        perror("rdmsr: open");
        exit(1);
    }
    covfd = open("coverage", O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);
    set_eflags_tf(1);
}

void trace_lbr_end() {
    set_eflags_tf(0);
    wrmsr(0x1d9, 0);
    close(msrfd);
    close(covfd);
}

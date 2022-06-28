/*
    SPDX-License-Identifier: GPL-2.0-or-later

    UMKa - User-Mode KolibriOS developer tools
    umka_os - kind of KolibriOS rump kernel

    Copyright (C) 2018-2022  Ivan Baravy <dunkaist@gmail.com>
*/

#include <arpa/inet.h>
#include <stdlib.h>
#include <fcntl.h>
#include <netinet/in.h>
#define __USE_GNU
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#define __USE_MISC
#include <sys/mman.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include "umka.h"
#include "shell.h"
#include "trace.h"
#include "vnet.h"

#define UMKA_DEFAULT_DISPLAY_WIDTH 400
#define UMKA_DEFAULT_DISPLAY_HEIGHT 300

#define THREAD_STACK_SIZE 0x100000

static void
monitor(void) {
    umka_sti();
    fprintf(stderr, "Start monitor thread\n");
    __asm__ __inline__ __volatile__ ("jmp $");
}

void umka_thread_net_drv(void);

struct itimerval timeout = {.it_value = {.tv_sec = 0, .tv_usec = 10000},
                            .it_interval = {.tv_sec = 0, .tv_usec = 10000}};

static void
thread_start(int is_kernel, void (*entry)(void), size_t stack_size) {
    fprintf(stderr, "### thread_start: %p\n", (void*)(uintptr_t)entry);
    uint8_t *stack = malloc(stack_size);
    umka_new_sys_threads(is_kernel, entry, stack + stack_size);
}

static void
dump_procs() {
    for (int i = 0; i < NR_SCHED_QUEUES; i++) {
        printf("sched queue #%i:", i);
        appdata_t *p_begin = kos_scheduler_current[i];
        appdata_t *p = p_begin;
        do {
            printf(" %p", (void*)p);
            p = p->in_schedule.next;
        } while (p != p_begin);
        putchar('\n');
    }
}

int
load_app_host(const char *fname, void *base) {
    FILE *f = fopen(fname, "r");
    if (!f) {
        perror("Can't open app file");
        exit(1);
    }
    fread(base, 1, 0x4000, f);
    fclose(f);

    for (size_t i = 0; i < 0x64; i++) {
        fprintf(stderr, "%2.2hx ", ((uint8_t*)base)[i]);
    }
    fprintf(stderr, "\n");

    return 0;
}

int
load_app(const char *fname) {
    int32_t result = umka_fs_execute(fname);
    printf("result: %" PRIi32 "\n", result);

    return result;
}

void handle_i40(int signo, siginfo_t *info, void *context) {
    (void)signo;
    (void)info;
    ucontext_t *ctx = context;
    void *ip = (void*)ctx->uc_mcontext.__gregs[REG_EIP];
    int eax = ctx->uc_mcontext.__gregs[REG_EAX];
    if (*(uint16_t*)ip == 0x40cd) {
        ctx->uc_mcontext.__gregs[REG_EIP] += 2; // skip int 0x40
    }
    printf("i40: %i %p\n", eax, ip);
    umka_i40((pushad_t*)(ctx->uc_mcontext.__gregs + REG_EDI));
}

void handle_irq_net(int signo, siginfo_t *info, void *context) {
    (void)signo;
    (void)info;
    (void)context;
    kos_irq_serv_irq10();
}

int
main() {
    if (coverage)
        trace_begin();

    umka_tool = UMKA_OS;
    umka_sti();

    struct sigaction sa;
    sa.sa_sigaction = irq0;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_SIGINFO;

    if (sigaction(SIGPROF, &sa, NULL) == -1) {
        printf("Can't install SIGPROF handler!\n");
        return 1;
    }

    sa.sa_sigaction = handle_i40;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_SIGINFO;

    if (sigaction(SIGSEGV, &sa, NULL) == -1) {
        printf("Can't install SIGSEGV handler!\n");
        return 1;
    }

    sa.sa_sigaction = handle_irq_net;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_SIGINFO;

    if (sigaction(SIGUSR1, &sa, NULL) == -1) {
        printf("Can't install SIGUSR1 handler!\n");
        return 1;
    }

    void *app_base = mmap((void*)0, 16*0x100000, PROT_READ | PROT_WRITE |
                          PROT_EXEC, MAP_FIXED | MAP_PRIVATE | MAP_ANONYMOUS,
                          -1, 0);
    if (app_base == MAP_FAILED) {
        perror("mmap failed");
        exit(1);
    }

    printf("pid=%d, kos_lfb_base=%p\n", getpid(), (void*)kos_lfb_base);

    kos_boot.bpp = 32;
    kos_boot.x_res = UMKA_DEFAULT_DISPLAY_WIDTH;
    kos_boot.y_res = UMKA_DEFAULT_DISPLAY_HEIGHT;
    kos_boot.pitch = UMKA_DEFAULT_DISPLAY_WIDTH*4;  // 32bpp

    umka_init();
    umka_stack_init();

    FILE *f = fopen("../img/kolibri.img", "r");
    fread(kos_ramdisk, 2880*512, 1, f);
    fclose(f);
    kos_ramdisk_init();
//    load_app_host("../apps/board_cycle", app_base);
//    load_app("/rd/1/loader");

    net_device_t *vnet = vnet_init();
    kos_net_add_device(vnet);

    char devname[64];
    for (size_t i = 0; i < umka_sys_net_get_dev_count(); i++) {
        umka_sys_net_dev_reset(i);
        umka_sys_net_get_dev_name(i, devname);
        uint32_t devtype = umka_sys_net_get_dev_type(i);
        fprintf(stderr, "[net_drv] device %i: %s %u\n", i, devname, devtype);
    }

    // network setup should be done from the userspace app, e.g. via zeroconf
    f76ret_t r76;
    r76 = umka_sys_net_ipv4_set_subnet(1, inet_addr("255.255.255.0"));
    if (r76.eax == (uint32_t)-1) {
        fprintf(stderr, "[net_drv] set subnet error\n");
        return -1;
    }

    r76 = umka_sys_net_ipv4_set_gw(1, inet_addr("10.50.0.1"));
    if (r76.eax == (uint32_t)-1) {
        fprintf(stderr, "set gw error\n");
        return -1;
    }

    r76 = umka_sys_net_ipv4_set_dns(1, inet_addr("217.10.36.5"));
    if (r76.eax == (uint32_t)-1) {
        fprintf(stderr, "[net_drv] set dns error\n");
        return -1;
    }

    r76 = umka_sys_net_ipv4_set_addr(1, inet_addr("10.50.0.2"));
    if (r76.eax == (uint32_t)-1) {
        fprintf(stderr, "[net_drv] set ip addr error\n");
        return -1;
    }


    thread_start(0, monitor, THREAD_STACK_SIZE);

    dump_procs();

    setitimer(ITIMER_PROF, &timeout, NULL);

    kos_osloop();   // doesn't return

    if (coverage)
        trace_end();

    return 0;
}

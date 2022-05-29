/*
    UMKa - User-Mode KolibriOS developer tools
    umka_os - kind of KolibriOS rump kernel
    Copyright (C) 2018--2021  Ivan Baravy <dunkaist@gmail.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#include <stdlib.h>
#include <fcntl.h>
#define __USE_GNU
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#define __USE_MISC
#include <sys/mman.h>
#include <sys/select.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include "umka.h"
#include "shell.h"
#include "trace.h"

#define UMKA_DEFAULT_DISPLAY_WIDTH 400
#define UMKA_DEFAULT_DISPLAY_HEIGHT 300

#define THREAD_STACK_SIZE 0x100000

static void
monitor(void) {
    umka_sti();
    fprintf(stderr, "Start monitor thread\n");
}

void umka_thread_ping(void);
void umka_thread_net_drv(void);

struct itimerval timeout = {.it_value = {.tv_sec = 0, .tv_usec = 10000},
                            .it_interval = {.tv_sec = 0, .tv_usec = 10000}};

static void
thread_start(int is_kernel, void (*entry)(void), size_t stack_size) {
    uint8_t *stack = malloc(stack_size);
    umka_new_sys_threads(is_kernel, entry, stack + stack_size);
}

/*
can't get pty working
may be because of my custom threads and blocking, don't know
void new_monitor(void) {
    umka_sti();
    fprintf(stderr, "Start monitor thread\n");

    int mpty = posix_openpt(O_RDWR | O_NOCTTY);
    if (mpty == -1) {
        perror("open master pty");
        return;
    }
    if (grantpt(mpty) == -1) {
        perror("grantpt");
        return;
    }
    if (unlockpt(mpty) == -1) {
        perror("unlockpt");
        return;
    }
    char *spty_name = ptsname(mpty);
    if (spty_name == NULL) {
        perror("open slave pty");
        return;
    }
    fprintf(stderr, "[os] pty=%s\n", spty_name);
    FILE *fmpty = fdopen(mpty, "r+");
    if (fmpty == NULL) {
        perror("fdopen mpty");
        return;
    }
    run_test(ctx);
}
*/

struct app_menuet {
    char signature[8];      // MENUETXX
    uint32_t    version;    // 1
    void (*start) (void);   // start
};

int
load_app(const char *fname, void *base) {
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

    void *app_base = mmap((void*)0, 16*0x100000, PROT_READ | PROT_WRITE |
                          PROT_EXEC, MAP_FIXED | MAP_PRIVATE | MAP_ANONYMOUS,
                          -1, 0);
    if (app_base == MAP_FAILED) {
        perror("mmap failed");
        exit(1);
    }

    load_app("../apps/hello_board", app_base);

    printf("pid=%d, kos_lfb_base=%p\n", getpid(), (void*)kos_lfb_base);

    kos_boot.bpp = 32;
    kos_boot.x_res = UMKA_DEFAULT_DISPLAY_WIDTH;
    kos_boot.y_res = UMKA_DEFAULT_DISPLAY_HEIGHT;
    kos_boot.pitch = UMKA_DEFAULT_DISPLAY_WIDTH*4;  // 32bpp

    umka_init();
    umka_stack_init();

    thread_start(0, monitor, THREAD_STACK_SIZE);
    thread_start(0, umka_thread_net_drv, THREAD_STACK_SIZE);
    thread_start(0, ((struct app_menuet*)app_base)->start, THREAD_STACK_SIZE);
//    thread_start(0, umka_thread_ping, THREAD_STACK_SIZE);

    setitimer(ITIMER_PROF, &timeout, NULL);

    kos_osloop();   // doesn't return

    if (coverage)
        trace_end();

    return 0;
}

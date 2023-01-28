/*
    SPDX-License-Identifier: GPL-2.0-or-later

    UMKa - User-Mode KolibriOS developer tools
    umka_os - kind of KolibriOS anykernel

    Copyright (C) 2018-2023  Ivan Baravy <dunkaist@gmail.com>
*/

#include <arpa/inet.h>
#include <stdlib.h>
#include <fcntl.h>
#include <limits.h>
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
#define __USE_GNU
#include <sys/uio.h>
#include <threads.h>
#include <SDL2/SDL.h>
#include "umka.h"
#include "umka_os.h"
#include "bestline.h"
#include "optparse.h"
#include "shell.h"
#include "trace.h"
#include "vnet.h"

#define HIST_FILE_BASENAME ".umka_os.history"

#define THREAD_STACK_SIZE 0x100000

struct umka_os_ctx {
    struct umka_ctx *umka;
    struct umka_io *io;
    struct shell_ctx *shell;
};

struct shared_info sinfo;

char history_filename[PATH_MAX];

static int
hw_int_mouse(void *arg) {
    (void)arg;
    kos_set_mouse_data(0, -50, 50, 0, 0);
    return 1;   // our interrupt
}

struct umka_os_ctx *
umka_os_init() {
    struct umka_os_ctx *ctx = malloc(sizeof(struct umka_os_ctx));
    ctx->umka = umka_init(UMKA_OS);
    ctx->io = io_init(&ctx->umka->running);
    ctx->shell = shell_init(SHELL_LOG_NONREPRODUCIBLE, history_filename,
                            ctx->umka, ctx->io);
    return ctx;
}

void build_history_filename() {
    const char *dir_name;
    if (!(dir_name = getenv("HOME"))) {
        dir_name = ".";
    }
    sprintf(history_filename, "%s/%s", dir_name, HIST_FILE_BASENAME);
}

/*
static void
monitor(void) {
    umka_sti();
    fprintf(stderr, "Start monitor thread\n");
    __asm__ __inline__ __volatile__ ("jmp $");
}
*/

void umka_thread_net_drv(void);

struct itimerval timeout = {.it_value = {.tv_sec = 0, .tv_usec = 10000},
                            .it_interval = {.tv_sec = 0, .tv_usec = 10000}};

typedef void (*kos_thread_t)(void);

static void
thread_start(int is_kernel, void (*entry)(void), size_t stack_size) {
    fprintf(stderr, "### thread_start: %p\n", (void*)(uintptr_t)entry);
    uint8_t *stack = malloc(stack_size);
    umka_new_sys_threads(is_kernel, entry, stack + stack_size);
}

static void
dump_procs() {
    for (int i = 0; i < NR_SCHED_QUEUES; i++) {
        fprintf(stderr, "sched queue #%i:", i);
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
    fread(base, 1, 0x100000, f);
    fclose(f);

    for (size_t i = 0; i < 0x64; i++) {
        fprintf(stderr, "%2.2hx ", ((uint8_t*)base)[i]);
    }
    fprintf(stderr, "\n");

    return 0;
}

/*
static int
load_app(const char *fname) {
    int32_t result = umka_fs_execute(fname);
    printf("result: %" PRIi32 "\n", result);

    return result;
}
*/

static void
handle_i40(int signo, siginfo_t *info, void *context) {
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

static void
handle_irq_net(int signo, siginfo_t *info, void *context) {
    (void)signo;
    (void)info;
    (void)context;
    kos_irq_serv_irq10();
}

static void
hw_int(int signo, siginfo_t *info, void *context) {
    (void)signo;
    (void)context;
    struct idt_entry *e = kos_idts + info->si_value.sival_int + 0x20;
    void (*handler)(void) = (void(*)(void)) (((uintptr_t)e->addr_hi << 16)
                                             + e->addr_lo);
    handler();
    umka_sti();
}

int
sdlthr(void *arg) {
    (void)arg;
    sigset_t set;
    pthread_sigmask(SIG_BLOCK, NULL, &set);
    pthread_sigmask(SIG_BLOCK, &set, NULL);
    if(SDL_Init(SDL_INIT_VIDEO) < 0)
    {
        fprintf(stderr, "Failed to initialize the SDL2 library\n");
        return -1;
    }

    SDL_Window *window = SDL_CreateWindow("LFB Viewer SDL2",
                                          SDL_WINDOWPOS_CENTERED,
                                          SDL_WINDOWPOS_CENTERED,
                                          sinfo.lfb_width, sinfo.lfb_height,
                                          0);

    if(!window)
    {
        fprintf(stderr, "Failed to create window\n");
        return -1;
    }

    SDL_Surface *window_surface = SDL_GetWindowSurface(window);

    if(!window_surface)
    {
        fprintf(stderr, "Failed to get the surface from the window\n");
        return -1;
    }

    uint32_t *lfb = (uint32_t*)malloc(sinfo.lfb_width*sinfo.lfb_height*sizeof(uint32_t));

    struct iovec remote = {.iov_base = (void*)(uintptr_t)sinfo.lfb_base,
                           .iov_len = sinfo.lfb_width*sinfo.lfb_height*4};
    struct iovec local = {.iov_base = lfb,
                          .iov_len = sinfo.lfb_width*sinfo.lfb_height*4};


    uint32_t *p = window_surface->pixels;
    while (1) {
        process_vm_readv(sinfo.pid, &local, 1, &remote, 1, 0);
        memcpy(window_surface->pixels, lfb, 400*300*4);
        SDL_LockSurface(window_surface);
        for (ssize_t y = 0; y < window_surface->h; y++) {
            for (ssize_t x = 0; x < window_surface->w; x++) {
                p[y*window_surface->pitch/4+x] = lfb[y*400+x];
            }
        }
        SDL_UnlockSurface(window_surface);
        SDL_UpdateWindowSurface(window);
        sleep(1);
    }


    return 0;
}

int
umka_monitor(void *arg) {
    (void)arg;
    sigset_t set;
    pthread_sigmask(SIG_BLOCK, NULL, &set);
    pthread_sigmask(SIG_BLOCK, &set, NULL);
/*
    printf("read umka_os configuration:\n"
           "  pid: %d\n"
           "  lfb_base: %p\n"
           "  lfb_bpp: %u"
           "  lfb_width: %u"
           "  lfb_height: %u"
           "  cmd: %p\n",
           (pid_t)sinfo.pid, (void*)(uintptr_t)sinfo.lfb_base, sinfo.lfb_bpp,
           sinfo.lfb_width, sinfo.lfb_height, (void*)(uintptr_t)sinfo.cmd_buf);
*/
    union sigval sval = (union sigval){.sival_int = 14};

    while (1) {
//        getchar();
//        sigqueue(sinfo.pid, SIGUSR2, sval);
        pause();
    }
    return 0;
}

int
main(int argc, char *argv[]) {
    (void)argc;
    const char *usage = "umka_os [-i <infile>] [-o <outfile>]\n";
    if (coverage) {
        trace_begin();
    }

    umka_sti();

    const char *infile = NULL, *outfile = NULL;
    build_history_filename();

    struct optparse options;
    int opt;
    optparse_init(&options, argv);

    while ((opt = optparse(&options, "i:o:s:")) != -1) {
        switch (opt) {
        case 'i':
            infile = options.optarg;
            break;
        case 'o':
            outfile = options.optarg;
            break;
        default:
            fprintf(stderr, "bad option: %c\n", opt);
            fputs(usage, stderr);
            exit(1);
        }
    }

    if (infile && !freopen(infile, "r", stdin)) {
        fprintf(stderr, "[!] can't open file for reading: %s\n", infile);
        exit(1);
    }
    if (outfile && !freopen(outfile, "w", stdout)) {
        fprintf(stderr, "[!] can't open file for writing: %s\n", outfile);
        exit(1);
    }

    struct umka_os_ctx *ctx = umka_os_init();

    struct sigaction sa;
    sa.sa_sigaction = irq0;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_SIGINFO;

    if (sigaction(SIGALRM, &sa, NULL) == -1) {
        fprintf(stderr, "Can't install SIGALRM handler!\n");
        return 1;
    }

    sa.sa_sigaction = handle_i40;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_SIGINFO;

    if (sigaction(SIGSEGV, &sa, NULL) == -1) {
        fprintf(stderr, "Can't install SIGSEGV handler!\n");
        return 1;
    }

    sa.sa_sigaction = handle_irq_net;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_SIGINFO;

    if (sigaction(SIGUSR1, &sa, NULL) == -1) {
        fprintf(stderr, "Can't install SIGUSR1 handler!\n");
        return 1;
    }

    sa.sa_sigaction = hw_int;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_SIGINFO;

    if (sigaction(SIGUSR2, &sa, NULL) == -1) {
        fprintf(stderr, "Can't install SIGUSR2 handler!\n");
        return 1;
    }

    struct app_hdr *app = mmap(KOS_APP_BASE, 16*0x100000, PROT_READ | PROT_WRITE
        | PROT_EXEC, MAP_FIXED | MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (app == MAP_FAILED) {
        perror("mmap failed");
        exit(1);
    }

    kos_boot.bpp = UMKA_DEFAULT_DISPLAY_BPP;
    kos_boot.x_res = UMKA_DEFAULT_DISPLAY_WIDTH;
    kos_boot.y_res = UMKA_DEFAULT_DISPLAY_HEIGHT;
    kos_boot.pitch = UMKA_DEFAULT_DISPLAY_WIDTH * UMKA_DEFAULT_DISPLAY_BPP / 8;

    sinfo = (struct shared_info) {
        .pid = getpid(),
        .lfb_base = (uintptr_t)kos_lfb_base,
        .lfb_bpp = kos_boot.bpp,
        .lfb_width = kos_boot.x_res,
        .lfb_height = kos_boot.y_res,
        .cmd_buf = (uintptr_t)cmd_buf,
    };
//    printf("pid=%d, kos_lfb_base=%p\n", getpid(), (void*)kos_lfb_base);

    run_test(ctx->shell);
//    umka_stack_init();

//    load_app_host("../apps/board_cycle", app);
    load_app_host("../apps/readdir", app);
//    load_app("/rd/1/loader");

//    net_device_t *vnet = vnet_init();
//    kos_net_add_device(vnet);

    char devname[64];
    for (size_t i = 0; i < umka_sys_net_get_dev_count(); i++) {
        umka_sys_net_dev_reset(i);
        umka_sys_net_get_dev_name(i, devname);
        uint32_t devtype = umka_sys_net_get_dev_type(i);
        fprintf(stderr, "[net_drv] device %i: %s %u\n", i, devname, devtype);
    }

/*
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
*/

    kos_attach_int_handler(14, hw_int_mouse, NULL);

//    thread_start(0, monitor, THREAD_STACK_SIZE);
    kos_thread_t start = (kos_thread_t)(KOS_APP_BASE + app->menuet.start);
    thread_start(0, start, THREAD_STACK_SIZE);

    dump_procs();

    fflush(stdin);
    fflush(stdout);
    fflush(stderr);
    thrd_t thrd_monitor;
    thrd_create(&thrd_monitor, umka_monitor, NULL);

    thrd_t thrd_screen;
    thrd_create(&thrd_screen, sdlthr, NULL);

    setitimer(ITIMER_REAL, &timeout, NULL);

    ctx->umka->running = 1;
    umka_osloop();   // doesn't return

    if (coverage)
        trace_end();

    return 0;
}

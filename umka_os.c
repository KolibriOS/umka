/*
    SPDX-License-Identifier: GPL-2.0-or-later

    UMKa - User-Mode KolibriOS developer tools
    umka_os - kind of KolibriOS anykernel

    Copyright (C) 2018-2023  Ivan Baravy <dunkaist@gmail.com>
*/

#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#define __USE_GNU
#include <signal.h>
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#define __USE_MISC
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <SDL2/SDL.h>
#include "umka.h"
#include "umkart.h"
#include "shell.h"
#include "trace.h"
#include "vnet.h"
#include "isocline/include/isocline.h"
#include "optparse/optparse.h"

#define HIST_FILE_BASENAME ".umka_os.history"

#define APP_MAX_MEM_SIZE 0x1000000
#define UMKA_LDR_BASE ((void*)0x1000000)

struct umka_os_ctx {
    struct umka_ctx *umka;
    struct umka_io *io;
    struct shell_ctx *shell;
    FILE *fboardlog;
};

struct umka_os_ctx *os;

char history_filename[PATH_MAX];

static int
hw_int_mouse(void *arg) {
    (void)arg;
    kos_set_mouse_data(0, -50, 50, 0, 0);
    return 1;   // our interrupt
}

struct umka_os_ctx *
umka_os_init(FILE *fstartup, FILE *fboardlog) {
    struct umka_os_ctx *ctx = malloc(sizeof(struct umka_os_ctx));
    ctx->fboardlog = fboardlog;
    ctx->umka = umka_init(UMKA_RUNNING_NOT_YET);
    ctx->io = io_init(&ctx->umka->running);
    ctx->shell = shell_init(SHELL_LOG_NONREPRODUCIBLE, history_filename,
                            ctx->umka, ctx->io, fstartup);
    return ctx;
}

static void
build_history_filename(void) {
    const char *dir_name;
    if (!(dir_name = getenv("HOME"))) {
        dir_name = ".";
    }
    sprintf(history_filename, "%s/%s", dir_name, HIST_FILE_BASENAME);
}

struct itimerval timeout = {.it_value = {.tv_sec = 0, .tv_usec = 10000},
                            .it_interval = {.tv_sec = 0, .tv_usec = 10000}};

static void
thread_start(int is_kernel, kos_thread_t entry, size_t stack_size) {
    fprintf(stderr, "[os] starting thread: %p\n", (void*)(uintptr_t)entry);
    uint8_t *stack = malloc(stack_size);
    kos_new_sys_threads(is_kernel, entry, stack + stack_size);
}

static void
dump_procs(void) {
    for (int i = 0; i < KOS_NR_SCHED_QUEUES; i++) {
        fprintf(stderr, "[os] sched queue #%i:", i);
        appdata_t *p_begin = kos_scheduler_current[i];
        appdata_t *p = p_begin;
        do {
            fprintf(stderr, " %p", (void*)p);
            p = p->in_schedule.next;
        } while (p != p_begin);
        putchar('\n');
    }
}

int
load_app_host(const char *fname, struct app_hdr *app) {
    FILE *f = fopen(fname, "rb");
    if (!f) {
        fprintf(stderr, "[!] can't open app file: %s", fname);
        exit(1);
    }
    fread(app, 1, APP_MAX_MEM_SIZE, f);
    fclose(f);

    kos_thread_t start = (kos_thread_t)(app->menuet.start);
    thread_start(0, start, UMKA_DEFAULT_THREAD_STACK_SIZE);
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
    fprintf(os->fboardlog, "i40: %i %p\n", eax, ip);
    umka_i40((pushad_t*)(ctx->uc_mcontext.__gregs + REG_EDI));
}

static void
hw_int(int signo) {
    (void)signo;
    size_t irq = atomic_load_explicit(&umka_irq_number, memory_order_acquire);
    struct idt_entry *e = kos_idts + UMKA_IRQ_BASE + irq;
    uintptr_t handler_addr = ((uintptr_t)e->addr_hi << 16) + e->addr_lo;
    void (*irq_handler)(void) = (void(*)(void)) handler_addr;
    irq_handler();
    umka_sti();
}

static void *
umka_display(void *arg) {
    (void)arg;
    if(SDL_Init(SDL_INIT_VIDEO) < 0)
    {
        fprintf(stderr, "Failed to initialize the SDL2 library\n");
        return NULL;
    }

    char title[64];
    sprintf(title, "umka0 %ux%u %ubpp", kos_display.width, kos_display.height,
            kos_display.bits_per_pixel);
    SDL_Window *window = SDL_CreateWindow(title,
                                          SDL_WINDOWPOS_UNDEFINED,
                                          SDL_WINDOWPOS_UNDEFINED,
                                          kos_display.width,
                                          kos_display.height,
                                          0);

    if(!window)
    {
        fprintf(stderr, "Failed to create window\n");
        return NULL;
    }

    SDL_Surface *window_surface = SDL_GetWindowSurface(window);

    if(!window_surface)
    {
        fprintf(stderr, "Failed to get the surface from the window\n");
        return NULL;
    }

    void (*copy_display)(void *) = NULL;

    switch (window_surface->format->format) {
    case SDL_PIXELFORMAT_RGB888:
        copy_display = copy_display_to_rgb888;
        break;
    default:
        printf("unknown SDL_PIXELFORMAT_* value: 0x%8.8x\n",
               window_surface->format->format);
        break;
    }

    while (1) {
        SDL_LockSurface(window_surface);
        copy_display(window_surface->pixels);
        SDL_UnlockSurface(window_surface);
        SDL_UpdateWindowSurface(window);
        sleep(1);
    }
    return NULL;
}

static void *
umka_monitor(void *arg) {
    struct shell_ctx *sh = arg;
    run_test(sh);
    exit(0);
}

static void
umka_thread_board(void) {
    struct board_get_ret c;
    while (1) {
        c = umka_sys_board_get();
        if (c.status) {
            fprintf(os->fboardlog, "%c", c.value);
        } else {
            umka_sys_csleep(50);
        }
    }
}

int
main(int argc, char *argv[]) {
    (void)argc;
    const char *usage = "umka_os [-i <infile>] [-o <outfile>]"
                        " [-b <boardlog>] [-s <startupfile>]\n";
    if (coverage) {
        trace_begin();
    }

    int show_display = 0;

    umka_sti();

    build_history_filename();

    const char *startupfile = NULL;
    const char *infile = NULL;
    const char *outfile = NULL;
    const char *boardlogfile = NULL;
    FILE *fstartup = NULL;
    FILE *fin = stdin;
    FILE *fout = stdout;
    FILE *fboardlog;

    struct optparse options;
    int opt;
    optparse_init(&options, argv);

    while ((opt = optparse(&options, "b:di:o:s:")) != -1) {
        switch (opt) {
        case 'b':
            boardlogfile = options.optarg;
            break;
        case 'd':
            show_display = 1;
            break;
        case 'i':
            infile = options.optarg;
            break;
        case 'o':
            outfile = options.optarg;
            break;
        case 's':
            startupfile = options.optarg;
            break;
        default:
            fprintf(stderr, "bad option: %c\n", opt);
            fputs(usage, stderr);
            exit(1);
        }
    }

    if (startupfile) {
        fstartup = fopen(startupfile, "rb");
        if (!fstartup) {
            fprintf(stderr, "[!] can't open file for reading: %s\n",
                    startupfile);
            exit(1);
        }
    }
    if (infile) {
        fin = fopen(infile, "rb");
        if (!fin) {
            fprintf(stderr, "[!] can't open file for reading: %s\n", infile);
            exit(1);
        }
    }
    if (outfile) {
        fout = fopen(outfile, "wb");
        if (!fout) {
            fprintf(stderr, "[!] can't open file for writing: %s\n", outfile);
            exit(1);
        }
    }
    if (boardlogfile) {
        fboardlog = fopen(boardlogfile, "wb");
        if (!fboardlog) {
            fprintf(stderr, "[!] can't open file for writing: %s\n",
                    boardlogfile);
            exit(1);
        }
    } else {
        fboardlog = fout;
    }

    os = umka_os_init(fstartup, fboardlog);

    struct sigaction sa;
    sa.sa_sigaction = irq0;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_SIGINFO | SA_RESTART;

    if (sigaction(SIGALRM, &sa, NULL) == -1) {
        fprintf(stderr, "Can't install timer interrupt handler!\n");
        return 1;
    }

    sa.sa_sigaction = handle_i40;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_SIGINFO | SA_NODEFER | SA_RESTART;

    if (sigaction(SIGSEGV, &sa, NULL) == -1) {
        fprintf(stderr, "Can't install 0x40 interrupt handler!\n");
        return 1;
    }

    sa.sa_handler = hw_int;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;

    if (sigaction(UMKA_SIGNAL_IRQ, &sa, NULL) == -1) {
        fprintf(stderr, "Can't install hardware interrupt handler!\n");
        return 1;
    }

    struct app_hdr *app_std = mmap(KOS_APP_BASE, APP_MAX_MEM_SIZE, PROT_READ
        | PROT_WRITE | PROT_EXEC, MAP_FIXED | MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (app_std == MAP_FAILED) {
        perror("mmap app_std failed");
        exit(1);
    }
    memset((void*)app_std, 0, APP_MAX_MEM_SIZE);
    struct app_hdr *app_ldr = mmap(UMKA_LDR_BASE, APP_MAX_MEM_SIZE, PROT_READ
        | PROT_WRITE | PROT_EXEC, MAP_FIXED | MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (app_ldr == MAP_FAILED) {
        perror("mmap app_ldr failed");
        exit(1);
    }
    memset((void*)app_ldr, 0, APP_MAX_MEM_SIZE);

    kos_boot.bpp = UMKA_DEFAULT_DISPLAY_BPP;
    kos_boot.x_res = UMKA_DEFAULT_DISPLAY_WIDTH;
    kos_boot.y_res = UMKA_DEFAULT_DISPLAY_HEIGHT;
    kos_boot.pitch = UMKA_DEFAULT_DISPLAY_WIDTH * UMKA_DEFAULT_DISPLAY_BPP / 8;

    run_test(os->shell);
    os->shell->fin = fin;
    clearerr(stdin);    // reset feof

//    load_app("/rd/1/loader");

    struct vnet *vnet = vnet_init(VNET_DEVTYPE_TAP, &os->umka->running);
    if (vnet) {
        kos_net_add_device(&vnet->eth.net);
    } else {
        fprintf(stderr, "[!] can't initialize vnet device\n");
    }

    char devname[64];
    for (size_t i = 0; i < umka_sys_net_get_dev_count(); i++) {
        umka_sys_net_dev_reset(i);
        umka_sys_net_get_dev_name(i, devname);
        uint32_t devtype = umka_sys_net_get_dev_type(i);
        fprintf(stderr, "[!] device %i: %s %u\n", i, devname, devtype);
    }

    // network setup should be done from the userspace app, e.g. via zeroconf
    f76ret_t r76;
    r76 = umka_sys_net_ipv4_set_subnet(1, inet_addr("255.255.255.0"));
    if (r76.eax == (uint32_t)-1) {
        fprintf(stderr, "[!] set subnet error\n");
//        return -1;
    }

    r76 = umka_sys_net_ipv4_set_gw(1, inet_addr("10.50.0.1"));
    if (r76.eax == (uint32_t)-1) {
        fprintf(stderr, "[!] set gw error\n");
//        return -1;
    }

    r76 = umka_sys_net_ipv4_set_dns(1, inet_addr("192.168.1.1"));
    if (r76.eax == (uint32_t)-1) {
        fprintf(stderr, "[!] set dns error\n");
//        return -1;
    }

    r76 = umka_sys_net_ipv4_set_addr(1, inet_addr("10.50.0.2"));
    if (r76.eax == (uint32_t)-1) {
        fprintf(stderr, "[!] set ip addr error\n");
//        return -1;
    }

    kos_attach_int_handler(UMKA_IRQ_MOUSE, hw_int_mouse, NULL);

    thread_start(1, umka_thread_board, UMKA_DEFAULT_THREAD_STACK_SIZE);
    load_app_host("../apps/loader", UMKA_LDR_BASE);
//    load_app_host("../apps/justawindow", KOS_APP_BASE);
    load_app_host("../apps/asciivju", KOS_APP_BASE);

    dump_procs();

    pthread_t thread_monitor;
    pthread_create(&thread_monitor, NULL, umka_monitor, os->shell);

    if (show_display) {
        pthread_t thread_display;
        pthread_create(&thread_display, NULL, umka_display, NULL);
    }

    setitimer(ITIMER_REAL, &timeout, NULL);

    atomic_store_explicit(&os->umka->running, UMKA_RUNNING_YES, memory_order_release);
    umka_osloop();   // doesn't return

    if (coverage)
        trace_end();

    return 0;
}

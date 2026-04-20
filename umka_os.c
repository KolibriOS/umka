/*
    SPDX-License-Identifier: GPL-2.0-or-later

    UMKa - User-Mode KolibriOS developer tools
    umka_os - kind of KolibriOS anykernel

    Copyright (C) 2018-2025  Ivan Baravy <dunkaist@gmail.com>
*/

#define _GNU_SOURCE
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <signal.h>
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <SDL3/SDL.h>
#include "umka.h"
#include "umkart.h"
#include "shell.h"
#include "trace.h"
#include "vnet.h"
#include "vkbd.h"
#include "isocline/include/isocline.h"
#include "optparse/optparse.h"

#define HIST_FILE_BASENAME ".umka_os.history"

#define APP_MAX_MEM_SIZE 0x1000000
#define UMKA_LDR_BASE ((void*)0x1000000)

struct umka_os_ctx {
    struct umka_ctx *umka;
    struct umka_io *io;
    struct monitor_ctx *monitor;
    struct shell_ctx *shell;
    FILE *fboardlog;
};

struct umka_os_ctx *os;
struct vkbd *vkbd;

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
    ctx->monitor = monitor_init(&ctx->umka->running);
    ctx->shell = shell_init(SHELL_LOG_NONREPRODUCIBLE, history_filename,
                            ctx->umka, ctx->io, ctx->monitor, fstartup);
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
        struct appdata *p_begin = kos_scheduler_current[i];
        struct appdata *p = p_begin;
        do {
            fprintf(stderr, " %p", (void*)p);
            p = p->in_schedule.next;
        } while (p != p_begin);
        putchar('\n');
    }
    for (size_t i = 0; i < 256; i++) {
        struct appdata *app = kos_slot_base + i;
        if (app->state != KOS_TSTATE_FREE && app->app_name[0]) {
            printf("slot %2.2d: %s\n", i, app->app_name);
        }
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
    void *ip = (void*)ctx->uc_mcontext.gregs[REG_EIP];
    int eax = ctx->uc_mcontext.gregs[REG_EAX];
    if (*(uint16_t*)ip == 0x40cd) {
        ctx->uc_mcontext.gregs[REG_EIP] += 2; // skip int 0x40
    }
    fprintf(os->fboardlog, "i40: %i %p\n", eax, ip);
    umka_i40((pushad_t*)(ctx->uc_mcontext.gregs + REG_EDI));
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

void (*copy_display)(void *);

static void
update_display(SDL_Surface *window_surface, SDL_Window *window) {
    SDL_LockSurface(window_surface);
    copy_display(window_surface->pixels);
    SDL_UnlockSurface(window_surface);
    SDL_UpdateWindowSurface(window);
    return;
}

struct {
    SDL_Scancode sdl_code;
    bool ps2_ext;
    uint8_t ps2_code;
} hid_to_ps2set1[] = {
                      {SDL_SCANCODE_COMMA,     false, 0x33},
                      {SDL_SCANCODE_PERIOD,    false, 0x34},
                      {SDL_SCANCODE_SLASH,     false, 0x35},
                      {SDL_SCANCODE_CAPSLOCK,  false, 0x3a},
                      {SDL_SCANCODE_HOME,       true, 0x47},
                      {SDL_SCANCODE_PAGEUP,     true, 0x49},
                      {SDL_SCANCODE_DELETE,     true, 0x53},
                      {SDL_SCANCODE_END,        true, 0x4f},
                      {SDL_SCANCODE_PAGEDOWN,   true, 0x51},
                      {SDL_SCANCODE_RIGHT,      true, 0x4d},
                      {SDL_SCANCODE_LEFT,       true, 0x4b},
                      {SDL_SCANCODE_DOWN,       true, 0x50},
                      {SDL_SCANCODE_UP,         true, 0x48},
                      {SDL_SCANCODE_A,         false, 0x1e},
                      {SDL_SCANCODE_B,         false, 0x30},
                      {SDL_SCANCODE_C,         false, 0x2e},
                      {SDL_SCANCODE_D,         false, 0x20},
                      {SDL_SCANCODE_E,         false, 0x12},
                      {SDL_SCANCODE_F,         false, 0x21},
                      {SDL_SCANCODE_G,         false, 0x22},
                      {SDL_SCANCODE_H,         false, 0x23},
                      {SDL_SCANCODE_I,         false, 0x17},
                      {SDL_SCANCODE_J,         false, 0x24},
                      {SDL_SCANCODE_K,         false, 0x25},
                      {SDL_SCANCODE_L,         false, 0x26},
                      {SDL_SCANCODE_M,         false, 0x32},
                      {SDL_SCANCODE_N,         false, 0x31},
                      {SDL_SCANCODE_O,         false, 0x18},
                      {SDL_SCANCODE_P,         false, 0x19},
                      {SDL_SCANCODE_Q,         false, 0x10},
                      {SDL_SCANCODE_R,         false, 0x13},
                      {SDL_SCANCODE_S,         false, 0x1f},
                      {SDL_SCANCODE_T,         false, 0x14},
                      {SDL_SCANCODE_U,         false, 0x16},
                      {SDL_SCANCODE_V,         false, 0x2f},
                      {SDL_SCANCODE_W,         false, 0x11},
                      {SDL_SCANCODE_X,         false, 0x2d},
                      {SDL_SCANCODE_Y,         false, 0x15},
                      {SDL_SCANCODE_Z,         false, 0x2c},
                      {SDL_SCANCODE_1,         false, 0x02},
                      {SDL_SCANCODE_2,         false, 0x03},
                      {SDL_SCANCODE_3,         false, 0x04},
                      {SDL_SCANCODE_4,         false, 0x05},
                      {SDL_SCANCODE_5,         false, 0x06},
                      {SDL_SCANCODE_6,         false, 0x07},
                      {SDL_SCANCODE_7,         false, 0x08},
                      {SDL_SCANCODE_8,         false, 0x09},
                      {SDL_SCANCODE_9,         false, 0x0a},
                      {SDL_SCANCODE_0,         false, 0x0b},
                      {SDL_SCANCODE_RETURN,    false, 0x1c},
                      {SDL_SCANCODE_ESCAPE,    false, 0x01},
                      {SDL_SCANCODE_BACKSPACE, false, 0x0e},
                      {SDL_SCANCODE_TAB,       false, 0x0f},
                      {SDL_SCANCODE_SPACE,     false, 0x39},
                     };

static void
convert_scancode_hid_to_ps2(SDL_Scancode sdl_code, int is_release, uint8_t *ps2) {
    for (size_t i = 0; i < sizeof(hid_to_ps2set1) / sizeof(*hid_to_ps2set1); i++) {
        if (sdl_code == hid_to_ps2set1[i].sdl_code) {
            if (hid_to_ps2set1[i].ps2_ext) {
                *ps2++ = 0xe0;
            }
            *ps2 = hid_to_ps2set1[i].ps2_code;
            if (is_release) {
                *ps2 |= 0x80;
            }
            return;
        }
    }
//    fprintf(stderr, "[sdl] unknown hid scancode: %i 0x%x\n", hid, hid);
}

static uint32_t
mouse_btn_state_sdl_to_umka(uint32_t sdl_btn_state) {
    uint32_t btn_state = 0;
    if ((sdl_btn_state & SDL_BUTTON_LMASK)) {
        btn_state |= 0x01;
    }
    if ((sdl_btn_state & SDL_BUTTON_RMASK)) {
        btn_state |= 0x02;
    }
    if ((sdl_btn_state & SDL_BUTTON_MMASK)) {
        btn_state |= 0x04;
    }
    return btn_state;
}

static void
grab_sdl_window(SDL_Window *window, bool grab) {
    SDL_SetWindowMouseGrab(window, grab);
    SDL_SetWindowKeyboardGrab(window, grab);
    SDL_SetWindowRelativeMouseMode(window, grab);
}

static void *
umka_display(void *arg) {
    (void)arg;
    if(!SDL_Init(SDL_INIT_VIDEO))
    {
        fprintf(stderr, "Failed to initialize SDL library\n");
        return NULL;
    }

    char title[64];
    sprintf(title, "umka %ux%u %ubpp, press Ctrl-Alt-g to (un)grab input",
            kos_display.width, kos_display.height, kos_display.bits_per_pixel);
    SDL_Window *window = SDL_CreateWindow(title,
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

    if (window_surface->format == SDL_PIXELFORMAT_XRGB8888) {
        copy_display = copy_display_to_xrgb8888;
    } else {
        printf("unknown SDL_PIXELFORMAT_* value: 0x%8.8x\n",
               window_surface->format);
    }

    bool input_grabbed = false;

    SDL_Event event;
    while (1) {
        update_display(window_surface, window);
        if (SDL_WaitEventTimeout(&event, 1000 /* ms */)) {
            switch (event.type) {
            case SDL_EVENT_QUIT:
                break;
            case SDL_EVENT_WINDOW_SHOWN:
            case SDL_EVENT_WINDOW_HIDDEN:
                break;
            case SDL_EVENT_MOUSE_BUTTON_DOWN:
                if (!input_grabbed) {
                    float fx, fy;
                    SDL_GetMouseState(&fx, &fy);
                    int x = (int)fx, y = (int)fy;
                    monitor_cmd_sys_set_mouse_pos_screen(os->monitor, x, y);
                    input_grabbed = true;
                    grab_sdl_window(window, true);
                }
                [[fallthrough]];
            case SDL_EVENT_MOUSE_BUTTON_UP: {
                if (!input_grabbed) {
                    break;
                }
                uint32_t btn_state =
                    mouse_btn_state_sdl_to_umka(SDL_GetMouseState(NULL, NULL));
                monitor_cmd_mouse_move(os->monitor, btn_state, 0, 0, 0, 0);
                break;
            }
            case SDL_EVENT_MOUSE_MOTION: {
                if (!input_grabbed) {
                    break;
                }
                uint32_t btn_state =
                    mouse_btn_state_sdl_to_umka(SDL_GetMouseState(NULL, NULL));
                monitor_cmd_mouse_move(os->monitor, btn_state,
                                       event.motion.xrel, -event.motion.yrel,
                                       event.wheel.y, event.wheel.x);
                break;
            }
            case SDL_EVENT_MOUSE_WHEEL: {
                if (!input_grabbed) {
                    break;
                }
                uint32_t btn_state =
                    mouse_btn_state_sdl_to_umka(SDL_GetMouseState(NULL, NULL));
                monitor_cmd_mouse_move(os->monitor, btn_state, 0, 0,
                                       event.wheel.y, event.wheel.x);
                break;
            }
            case SDL_EVENT_KEY_DOWN: {
                if ((event.key.scancode == SDL_SCANCODE_G)
                    && (event.key.mod & SDL_KMOD_CTRL)
                    && (event.key.mod & SDL_KMOD_ALT)) {
                    input_grabbed = !input_grabbed;
                    grab_sdl_window(window, input_grabbed);
                }
/*
                if (!input_grabbed) {
                    break;
                }
*/
                uint8_t scancodes[16];
                memset(scancodes, 0, 16);
                convert_scancode_hid_to_ps2(event.key.scancode, false, scancodes);
                monitor_cmd_send_scancodes(os->monitor, scancodes);
                convert_scancode_hid_to_ps2(event.key.scancode, true, scancodes);
                monitor_cmd_send_scancodes(os->monitor, scancodes);
                break;
            }
            case SDL_EVENT_KEY_UP:
                if (!input_grabbed) {
                    break;
                }
                break;
            case SDL_EVENT_TEXT_INPUT:
                if (!input_grabbed) {
                    break;
                }
                break;
            case SDL_EVENT_WINDOW_EXPOSED:
            case SDL_EVENT_WINDOW_FOCUS_GAINED:
            case SDL_EVENT_WINDOW_FOCUS_LOST:
            case SDL_EVENT_WINDOW_MOUSE_ENTER:
            case SDL_EVENT_WINDOW_MOUSE_LEAVE:
            case SDL_EVENT_CLIPBOARD_UPDATE:    // TODO: pass through clipboard
                break;
            default:
//                fprintf(stderr, "[sdl] unknown event type: 0x%x\n", event.type);
                update_display(window_surface, window);
            }
        } else {
            update_display(window_surface, window);
        }
//        sleep(1);
    }
    return NULL;
}

static void *
umka_shell(void *arg) {
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
                        " [-s <startupfile>] [-b <boardlog>] [-d(isplay)]"
                        " [-c covfile]\n";

    int coverage = 0;
    int show_display = 0;

    umka_sti();

    build_history_filename();

    const char *startupfile = NULL;
    const char *infile = NULL;
    const char *outfile = NULL;
    const char *boardlogfile = NULL;
    const char *covfile = NULL;
    FILE *fstartup = NULL;
    FILE *fin = stdin;
    FILE *fout = stdout;
    FILE *fboardlog;

    struct optparse options;
    int opt;
    optparse_init(&options, argv);

    while ((opt = optparse(&options, "b:c:di:o:s:")) != -1) {
        switch (opt) {
        case 'b':
            boardlogfile = options.optarg;
            break;
        case 'c':
            coverage = 1;
            covfile = options.optarg;
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

    if (coverage) {
        trace_enable();
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

    run_test(os->shell);
    os->shell->fin = fin;
    clearerr(stdin);    // reset feof

//    load_app("/rd/1/loader");

    vkbd = vkbd_init();

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
    struct f76ret r76;
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
//    load_app_host("../apps/asciivju", KOS_APP_BASE);
    load_app_host("../apps/charsets", KOS_APP_BASE);
//    load_app_host("../apps/eyes", KOS_APP_BASE);
//    load_app_host("../apps/board", KOS_APP_BASE);
//    load_app_host("../apps/calc", KOS_APP_BASE);
//    load_app_host("../apps/unvwater", KOS_APP_BASE);
//    load_app_host("../apps/clicks", KOS_APP_BASE);
//    load_app_host("../apps/testcon", KOS_APP_BASE);

    dump_procs();

    pthread_t thread_monitor;
    pthread_create(&thread_monitor, NULL, umka_shell, os->shell);

    if (show_display) {
        pthread_t thread_display;
        pthread_create(&thread_display, NULL, umka_display, NULL);
    }

    atomic_store_explicit(&os->umka->running, UMKA_RUNNING_YES, memory_order_release);
    setitimer(ITIMER_REAL, &timeout, NULL);

    umka_osloop();   // doesn't return

    if (coverage)
        trace_disable();

    (void)covfile;

    return 0;
}

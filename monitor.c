/*
    SPDX-License-Identifier: GPL-2.0-or-later

    UMKa - User-Mode KolibriOS developer tools
    monitor - the command interface

    Copyright (C) 2025  Ivan Baravy <dunkaist@gmail.com>
*/

#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include "umka.h"
#include "monitor.h"
#include "trace.h"

#define MONITOR_CMD_BUF_LEN 0x10

enum {
    MONITOR_CMD_STATUS_EMPTY,
    MONITOR_CMD_STATUS_READY,
    MONITOR_CMD_STATUS_DONE,
};

struct umka_cmd umka_cmd_buf[MONITOR_CMD_BUF_LEN];

struct monitor_ctx *
monitor_init(atomic_int *running) {
    struct monitor_ctx *monitor = malloc(sizeof(struct monitor_ctx));
    monitor->running = running;
    monitor->fout = stdout;
    pthread_cond_init(&monitor->cmd_done, NULL);
    pthread_mutex_init(&monitor->cmd_mutex, NULL);
/*
    if (running) {
        pthread_create(&io->iot, NULL, thread_io, NULL);
    }
*/
    return monitor;
}

static void
thread_cmd_runner(void *arg);

void
monitor_cmd_boot(struct monitor_ctx *ctx) {
    if (*ctx->running != UMKA_RUNNING_NEVER) {
        char *stack = malloc(UMKA_DEFAULT_THREAD_STACK_SIZE);
        char *stack_top = stack + UMKA_DEFAULT_THREAD_STACK_SIZE;
        size_t tid = umka_new_sys_threads(0, thread_cmd_runner, stack_top, ctx,
                                          "cmd_runner");
        (void)tid;
    }
}

void
monitor_cmd_send_scancodes(struct monitor_ctx *mon, uint8_t *scancodes) {
    struct umka_cmd *cmd = monitor_get_cmd(mon);
    cmd->type = UMKA_CMD_SEND_SCANCODE;
    struct cmd_send_scancode_arg *c = &cmd->send_scancode.arg;
    for (uint8_t *sc = scancodes; *sc; sc++) {
        c->scancode = *sc;
        monitor_run_cmd(mon);
        monitor_clear_cmd(cmd);
    }
}

void
monitor_cmd_csleep(struct monitor_ctx *mon, uint32_t csec) {
    struct umka_cmd *cmd = monitor_get_cmd(mon);
    struct cmd_sys_csleep_arg *c = &cmd->sys_csleep.arg;
    cmd->type = UMKA_CMD_SYS_CSLEEP;
    c->csec = csec;
    monitor_run_cmd(mon);
    monitor_clear_cmd(cmd);
}

void
monitor_cmd_wait_for_idle(struct monitor_ctx *mon) {
    struct umka_cmd *cmd = monitor_get_cmd(mon);
    cmd->type = UMKA_CMD_WAIT_FOR_IDLE;
    monitor_run_cmd(mon);
    monitor_clear_cmd(cmd);
}

void
monitor_cmd_wait_for_os_idle(struct monitor_ctx *mon) {
    struct umka_cmd *cmd = monitor_get_cmd(mon);
    cmd->type = UMKA_CMD_WAIT_FOR_OS_IDLE;
    monitor_run_cmd(mon);
    monitor_clear_cmd(cmd);
}

void
monitor_cmd_wait_for_window(struct monitor_ctx *mon, char *wnd_title) {
    struct umka_cmd *cmd = monitor_get_cmd(mon);
    cmd->type = UMKA_CMD_WAIT_FOR_WINDOW;
    struct cmd_wait_for_window_arg *c = &cmd->wait_for_window.arg;
    c->wnd_title = wnd_title;
    monitor_run_cmd(mon);
    monitor_clear_cmd(cmd);
}

void
monitor_cmd_mouse_move(struct monitor_ctx *mon, uint32_t btn_state,
                       int32_t xmoving, int32_t ymoving, int32_t vscroll,
                       int32_t hscroll) {
    struct umka_cmd *cmd = monitor_get_cmd(mon);
    cmd->type = UMKA_CMD_SET_MOUSE_DATA;
    struct cmd_set_mouse_data_arg *c = &cmd->set_mouse_data.arg;
    c->btn_state = btn_state;
    c->xmoving = xmoving;
    c->ymoving = ymoving;
    c->vscroll = vscroll;
    c->hscroll = hscroll;
    monitor_run_cmd(mon);
    monitor_clear_cmd(cmd);
}

struct f7080ret
monitor_cmd_sys_lfn(struct monitor_ctx *mon, enum f70or80 f70or80,
                    union f7080arg *fX0) {
    struct umka_cmd *cmd = monitor_get_cmd(mon);
    struct cmd_sys_lfn_arg *c = &cmd->sys_lfn.arg;
    cmd->type = UMKA_CMD_SYS_LFN;
    c->f70or80 = f70or80;
    c->bufptr = fX0;
    struct f7080ret ret;
    c->r = &ret;
    monitor_run_cmd(mon);
    monitor_clear_cmd(cmd);
    return ret;
}

void
monitor_cmd_sys_set_mouse_pos_screen(struct monitor_ctx *mon, int16_t x,
                                     int16_t y) {
    struct umka_cmd *cmd = monitor_get_cmd(mon);
    cmd->type = UMKA_CMD_SYS_SET_MOUSE_POS_SCREEN;
    struct cmd_sys_set_mouse_pos_screen_arg *c = &cmd->sys_set_mouse_pos_screen.arg;
    struct point16s pos;
    pos.x = x;
    pos.y = y;
    c->pos = pos;
    monitor_run_cmd(mon);
    monitor_clear_cmd(cmd);
}

static uint32_t
umka_wait_for_idle_test(void) {
    return (uint32_t)(atomic_load_explicit(&idle_scheduled, memory_order_acquire));
}

static void
umka_wait_for_idle(void) {
    atomic_store_explicit(&idle_scheduled, 0, memory_order_release);
    kos_wait_events(umka_wait_for_idle_test, NULL);
}

static uint32_t
monitor_run_cmd_wait_test(void /* struct appdata * with wait_param is in ebx */) {
    struct appdata *app;
    __asm__ __volatile__ __inline__ ("":"=b"(app)::);
    struct umka_cmd *cmd = (struct umka_cmd*)app->wait_param;
    return (uint32_t)(atomic_load_explicit(&cmd->status, memory_order_acquire) == MONITOR_CMD_STATUS_READY);
}

static void
monitor_run_cmd_sync(struct monitor_ctx *ctx);

static void
thread_cmd_runner(void *arg) {
    umka_sti();
    struct monitor_ctx *ctx = arg;
    while (1) {
        kos_wait_events(monitor_run_cmd_wait_test, umka_cmd_buf);
        monitor_run_cmd_sync(ctx);
    }
}

static uint32_t
umka_wait_for_os_test(void) {
    return (uint32_t)(atomic_load_explicit(&os_scheduled, memory_order_acquire));
}

static void
umka_wait_for_os_idle(void) {
    atomic_store_explicit(&os_scheduled, 0, memory_order_release);
    kos_wait_events(umka_wait_for_os_test, NULL);
    atomic_store_explicit(&idle_scheduled, 0, memory_order_release);
    kos_wait_events(umka_wait_for_idle_test, NULL);
}

static uint32_t
umka_wait_for_window_test(void) {
    struct appdata *app;
    struct wdata *wdata;
    __asm__ __volatile__ __inline__ ("":"=b"(app)::);
    const char *wnd_title = (const char *)app->wait_param;
    for (size_t i = 0; i < 256; i++) {
        app = kos_slot_base + i;
        wdata = kos_window_data + i;
        if (app->state != KOS_TSTATE_FREE && wdata->caption
            && !strcmp(wdata->caption, wnd_title)) {
            return 1;
        }
    }
    return 0;
}

static void
umka_wait_for_window(char *wnd_title) {
    kos_wait_events(umka_wait_for_window_test, wnd_title);
}

static void
monitor_run_cmd_sync(struct monitor_ctx *mon) {
    struct umka_cmd *cmd = monitor_get_cmd(mon);
    switch (cmd->type) {
    case UMKA_CMD_WAIT_FOR_IDLE: {
        COVERAGE_ON();
        umka_wait_for_idle();
        COVERAGE_OFF();
        break;
        }
    case UMKA_CMD_WAIT_FOR_OS_IDLE: {
        COVERAGE_ON();
        umka_wait_for_os_idle();
        COVERAGE_OFF();
        break;
        }
    case UMKA_CMD_WAIT_FOR_WINDOW: {
        struct cmd_wait_for_window_arg *c = &cmd->wait_for_window.arg;
        COVERAGE_ON();
        umka_wait_for_window(c->wnd_title);
        COVERAGE_OFF();
        break;
        }
    case UMKA_CMD_SYS_CSLEEP: {
        struct cmd_sys_csleep_arg *c = &cmd->sys_csleep.arg;
        COVERAGE_ON();
        umka_sys_csleep(c->csec);
        COVERAGE_OFF();
        break;
        }
    case UMKA_CMD_SET_MOUSE_DATA: {
        struct cmd_set_mouse_data_arg *c = &cmd->set_mouse_data.arg;
        COVERAGE_ON();
        kos_set_mouse_data(c->btn_state, c->xmoving, c->ymoving, c->vscroll,
                           c->hscroll);
        COVERAGE_OFF();
        break;
        }
    case UMKA_CMD_SEND_SCANCODE: {
        struct cmd_send_scancode_arg *c = &cmd->send_scancode.arg;
        COVERAGE_ON();
        umka_set_keyboard_data(c->scancode);
        COVERAGE_OFF();
        break;
        }
    case UMKA_CMD_SYS_SET_MOUSE_POS_SCREEN: {
        struct cmd_sys_set_mouse_pos_screen_arg *c = &cmd->sys_set_mouse_pos_screen.arg;
        COVERAGE_ON();
        umka_sys_set_mouse_pos_screen(c->pos);
        COVERAGE_OFF();
        break;
        }
    case UMKA_CMD_SYS_LFN: {
        struct cmd_sys_lfn_arg *c = &cmd->sys_lfn.arg;
        COVERAGE_ON();
        umka_sys_lfn(c->bufptr, c->r, c->f70or80);
        COVERAGE_OFF();
        break;
        }
    default:
        fprintf(mon->fout, "[!monitor] unknown command: %u\n", cmd->type);
        break;
    }
    atomic_store_explicit(&cmd->status, MONITOR_CMD_STATUS_DONE,
                          memory_order_release);
    pthread_cond_signal(&mon->cmd_done);
}

struct umka_cmd *
monitor_get_cmd(struct monitor_ctx *monitor) {
    (void)monitor;
    return umka_cmd_buf;
}

void
monitor_run_cmd(struct monitor_ctx *ctx) {
    struct umka_cmd *cmd = umka_cmd_buf;
    atomic_store_explicit(&cmd->status, MONITOR_CMD_STATUS_READY,
                          memory_order_release);
    if (atomic_load_explicit(ctx->running, memory_order_acquire) == UMKA_RUNNING_YES) {
        pthread_cond_wait(&ctx->cmd_done, &ctx->cmd_mutex);
    } else {
        monitor_run_cmd_sync(ctx);
    }
}

void
monitor_clear_cmd(struct umka_cmd *cmd) {
    atomic_store_explicit(&cmd->status, MONITOR_CMD_STATUS_EMPTY,
                          memory_order_release);
}

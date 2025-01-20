/*
    SPDX-License-Identifier: GPL-2.0-or-later

    UMKa - User-Mode KolibriOS developer tools
    umka_shell - the shell

    Copyright (C) 2017-2023  Ivan Baravy <dunkaist@gmail.com>
    Copyright (C) 2021  Magomed Kostoev <mkostoevr@yandex.ru>
*/

#include <assert.h>
#include <errno.h>
#include <inttypes.h>
#include <limits.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

// TODO: Cleanup
#ifndef _WIN32
#include <arpa/inet.h>
#else
#include <winsock2.h>
#include <io.h>
#endif

#include "shell.h"
#include "vdisk.h"
#include "vnet.h"
#include "umka.h"
#include "trace.h"
#include "pci.h"
#include "umkart.h"
#include "lodepng/lodepng.h"
#include "optparse/optparse.h"
#include "isocline/include/isocline.h"

#define MAX_COMMAND_ARGS 42
#define PRINT_BYTES_PER_LINE 32
#define MAX_DIRENTS_TO_READ 100
#define MAX_BYTES_TO_READ (1024*1024)

#define DEFAULT_READDIR_ENCODING UTF8
#define DEFAULT_PATH_ENCODING UTF8

#define SHELL_CMD_BUF_LEN 0x10

enum {
    SHELL_CMD_STATUS_EMPTY,
    SHELL_CMD_STATUS_READY,
    SHELL_CMD_STATUS_DONE,
};

struct umka_cmd umka_cmd_buf[SHELL_CMD_BUF_LEN];

char prompt_line[PATH_MAX];
char cur_dir[PATH_MAX] = "/";
const char *last_dir = cur_dir;
bool cur_dir_changed = true;

typedef struct {
    char *name;
    void (*func) (struct shell_ctx *, int, char **);
} func_table_t;

static uint32_t
shell_run_cmd_wait_test(void /* struct appdata * with wait_param is in ebx */) {
    appdata_t *app;
    __asm__ __volatile__ __inline__ ("":"=b"(app)::);
    struct umka_cmd *cmd = (struct umka_cmd*)app->wait_param;
    return (uint32_t)(atomic_load_explicit(&cmd->status, memory_order_acquire) == SHELL_CMD_STATUS_READY);
}

static void
shell_run_cmd_sync(struct shell_ctx *ctx);

static void
thread_cmd_runner(void *arg) {
    umka_sti();
    struct shell_ctx *ctx = arg;
    while (1) {
        kos_wait_events(shell_run_cmd_wait_test, umka_cmd_buf);
        shell_run_cmd_sync(ctx);
    }
}

const char *f70_status_name[] = {
                                 "success",
                                 "disk_base",
                                 "unsupported_fs",
                                 "unknown_fs",
                                 "partition",
                                 "file_not_found",
                                 "end_of_file",
                                 "memory_pointer",
                                 "disk_full",
                                 "fs_fail",
                                 "access_denied",
                                 "device",
                                 "out_of_memory"
                                };

static const char *
get_f70_status_name(int s) {
    switch (s) {
    case KOS_ERROR_SUCCESS:
//        return "";
    case KOS_ERROR_DISK_BASE:
    case KOS_ERROR_UNSUPPORTED_FS:
    case KOS_ERROR_UNKNOWN_FS:
    case KOS_ERROR_PARTITION:
    case KOS_ERROR_FILE_NOT_FOUND:
    case KOS_ERROR_END_OF_FILE:
    case KOS_ERROR_MEMORY_POINTER:
    case KOS_ERROR_DISK_FULL:
    case KOS_ERROR_FS_FAIL:
    case KOS_ERROR_ACCESS_DENIED:
    case KOS_ERROR_DEVICE:
    case KOS_ERROR_OUT_OF_MEMORY:
        return f70_status_name[s];
    default:
        return "unknown";
    }
}

static void
convert_f70_file_attr(uint32_t attr, char s[KF_ATTR_CNT+1]) {
    s[0] = (attr & KF_READONLY) ? 'r' : '-';
    s[1] = (attr & KF_HIDDEN)   ? 'h' : '-';
    s[2] = (attr & KF_SYSTEM)   ? 's' : '-';
    s[3] = (attr & KF_LABEL)    ? 'l' : '-';
    s[4] = (attr & KF_FOLDER)   ? 'f' : '-';
    s[5] = '\0';
}

static void
print_f70_status(struct shell_ctx *ctx, f7080ret_t *r, int use_ebx) {
    fprintf(ctx->fout, "status = %d %s", r->status,
            get_f70_status_name(r->status));
    if (use_ebx
        && (r->status == KOS_ERROR_SUCCESS || r->status == KOS_ERROR_END_OF_FILE)) {
        fprintf(ctx->fout, ", count = %d", r->count);
    }
    fprintf(ctx->fout, "\n");
}

static bool
parse_uintmax(const char *str, uintmax_t *res) {
    char *endptr;
    *res = strtoumax(str, &endptr, 0);
    bool ok = (str != endptr) && (*endptr == '\0');
    return ok;
}

static bool
parse_uint32(struct shell_ctx *ctx, const char *str, uint32_t *res) {
    uintmax_t x;
    if (parse_uintmax(str, &x) && x <= UINT32_MAX) {
        *res = (uint32_t)x;
        return true;
    } else {
        fprintf(ctx->fout, "invalid number: %s\n", str);
        return false;
    }
}

static bool
parse_uint64(struct shell_ctx *ctx, const char *str, uint64_t *res) {
    uintmax_t x;
    if (parse_uintmax(str, &x) && x <= UINT64_MAX) {
        *res = x;
        return true;
    } else {
        fprintf(ctx->fout, "invalid number: %s\n", str);
        return false;
    }
}

static struct shell_var *
shell_var_get(struct shell_ctx *ctx, const char *name) {
    for (struct shell_var *var = ctx->var; var; var = var->next) {
        if (!strcmp(var->name, name)) {
            return var;
        }
    }
    return NULL;
}

static bool
shell_parse_sint(struct shell_ctx *ctx, ssize_t *value, const char *s) {
    if (s[0] == '$') {
        struct shell_var *var = shell_var_get(ctx, s);
        if (var) {
            *value = var->value.sint;
            return true;
        }
    } else {
        *value = strtol(s, NULL, 0);
        return true;
    }
    return false;
}

static bool
shell_parse_uint(struct shell_ctx *ctx, size_t *value, const char *s) {
    if (s[0] == '$') {
        struct shell_var *var = shell_var_get(ctx, s);
        if (var) {
            *value = var->value.uint;
            return true;
        }
    } else {
        *value = strtoul(s, NULL, 0);
        return true;
    }
    return false;
}

static bool
shell_parse_ptr(struct shell_ctx *ctx, void **value, const char *s) {
    if (s[0] == '$') {
        struct shell_var *var = shell_var_get(ctx, s);
        if (var) {
            *value = var->value.ptr;
            return true;
        }
    } else {
        *value = (void*)strtoul(s, NULL, 0);
        return true;
    }
    return false;
}

static bool
shell_var_name(struct shell_ctx *ctx, const char *name) {
    struct shell_var *var = ctx->var;
    if (!var || var->name[0] != '\0') {
        return false;
    }
    if (name[0] != '$' || strlen(name) >= SHELL_VAR_NAME_LEN) {
        return false;
    }
    strcpy(var->name, name);
    return true;
}

struct shell_var *
shell_var_new(void) {
    struct shell_var *var = (struct shell_var*)malloc(sizeof(struct shell_var));
    var->next = NULL;
    var->name[0] = '\0';
    return var;
}

static struct shell_var *
shell_var_add(struct shell_ctx *ctx) {
    struct shell_var *var = ctx->var;
    struct shell_var *new_var;
    if (!var) {
        new_var = shell_var_new();
        ctx->var = new_var;
    } else {
        if (var->name[0] == '\0') {
            new_var = var;
        } else {
            new_var = shell_var_new();
            new_var->next = var;
            ctx->var = new_var;
        }
    }
    return new_var;
}

static bool
shell_var_add_sint(struct shell_ctx *ctx, ssize_t value) {
    struct shell_var *var = shell_var_add(ctx);
    if (!var) {
        return false;
    }
    var->type = SHELL_VAR_SINT;
    var->value.sint = value;
    return true;
}

static bool
shell_var_add_uint(struct shell_ctx *ctx, size_t value) {
    struct shell_var *var = shell_var_add(ctx);
    if (!var) {
        return false;
    }
    var->type = SHELL_VAR_UINT;
    var->value.uint = value;
    return true;
}

static bool
shell_var_add_ptr(struct shell_ctx *ctx, void *value) {
    struct shell_var *var = shell_var_add(ctx);
    if (!var) {
        return false;
    }
    var->type = SHELL_VAR_POINTER;
    var->value.ptr = value;
    return true;
}

static void
print_bytes(struct shell_ctx *ctx, uint8_t *x, size_t len) {
    for (size_t i = 0; i < len; i++) {
        if (i % PRINT_BYTES_PER_LINE == 0 && i != 0) {
            fprintf(ctx->fout, "\n");
        }
        fprintf(ctx->fout, "%2.2x", x[i]);
    }
    fprintf(ctx->fout, "\n");
}

static void
print_hash(struct shell_ctx *ctx, uint8_t *x, size_t len) {
    hash_context hash;
    hash_oneshot(&hash, x, len);
    for (size_t i = 0; i < HASH_SIZE; i++) {
        fprintf(ctx->fout, "%2.2x", hash.hash[i]);
    }
    fprintf(ctx->fout, "\n");
}

void *host_load_file(const char *fname) {
    FILE *f = fopen(fname, "rb");
    if (!f) {
        return NULL;
    }
    fseek(f, 0, SEEK_END);
    size_t fsize = ftell(f);
    rewind(f);
    void *fdata = malloc(fsize);
    fread(fdata, 1, fsize, f);
    fclose(f);
    return fdata;
}

static const char *
get_last_dir(const char *path) {
    const char *last = strrchr(path, '/');
    if (!last) {
        last = path;
    } else if (last != path || last[1] != '\0') {
        last++;
    }
    return last;
}

static void
prompt(struct shell_ctx *ctx) {
    if (cur_dir_changed) {
        if (ctx->umka->booted) {
            COVERAGE_ON();
            umka_sys_get_cwd(cur_dir, PATH_MAX);
            COVERAGE_OFF();
        }
        last_dir = get_last_dir(cur_dir);
        cur_dir_changed = false;
    }
    fprintf(ctx->fout, "%s> ", last_dir);
    fflush(stdout);
}

static void
completer(ic_completion_env_t *cenv, const char *prefix) {
    (void)cenv;
    (void)prefix;
}

static void
highlighter(ic_highlight_env_t *henv, const char *input, void *arg) {
    (void)henv;
    (void)input;
    (void)arg;
}

static int
split_args(char *s, char **argv) {
    int argc = -1;
    for (; (argv[++argc] = strtok(s, " \t\n\r")) != NULL; s = NULL);
    return argc;
}

static void
cmd_var(struct shell_ctx *ctx, int argc, char **argv) {
    (void)ctx;
    const char *usage = \
        "usage: var <$name>\n"
        "  $name            variable name, must start with $ sign\n";
    if (argc != 2) {
        fputs(usage, ctx->fout);
        return;
    }
    bool status = shell_var_name(ctx, argv[1]);
    if (!status) {
        fprintf(ctx->fout, "fail\n");
    }
}

static void
cmd_send_scancode(struct shell_ctx *ctx, int argc, char **argv) {
    (void)ctx;
    const char *usage = \
        "usage: send_scancode <code>...\n"
        "  code             dec or hex number\n";
    if (argc < 2) {
        fputs(usage, ctx->fout);
        return;
    }
    argc -= 1;
    argv += 1;

    while (argc) {
        char *endptr;
        size_t code = strtoul(argv[0], &endptr, 0);
        if (*endptr == '\0') {
            umka_set_keyboard_data(code);
            argc--;
            argv++;
        } else {
            fprintf(ctx->fout, "not an integer: %s\n", argv[0]);
            fputs(usage, ctx->fout);
            return;
        }
    }
}

static void
cmd_umka_boot(struct shell_ctx *ctx, int argc, char **argv) {
    (void)ctx;
    (void)argv;
    const char *usage = \
        "usage: umka_boot\n";
    if (argc != 1) {
        fputs(usage, ctx->fout);
        return;
    }

    COVERAGE_ON();
    umka_boot();
    COVERAGE_OFF();

    if (*ctx->running != UMKA_RUNNING_NEVER) {
        char *stack = malloc(UMKA_DEFAULT_THREAD_STACK_SIZE);
        char *stack_top = stack + UMKA_DEFAULT_THREAD_STACK_SIZE;
        size_t tid = umka_new_sys_threads(0, thread_cmd_runner, stack_top, ctx,
                                          "cmd_runner");
        (void)tid;
    }
}

static void
cmd_umka_set_boot_params(struct shell_ctx *ctx, int argc, char **argv) {
    const char *usage = \
        "usage: umka_set_boot_params [--x_res <num>] [--y_res <num>]\n"
        "  --x_res <num>    screen width\n"
        "  --y_res <num>    screen height\n"
        "  --bpp <num>      screen bits per pixel\n"
        "  --pitch <num>    screen line length in bytes\n";
    argc -= 1;
    argv += 1;

    while (argc) {
        if (!strcmp(argv[0], "--x_res") && argc > 1) {
            kos_boot.x_res = strtoul(argv[1], NULL, 0);
            kos_boot.pitch = kos_boot.x_res * kos_boot.bpp/8;
            argc -= 2;
            argv += 2;
            continue;
        } else if (!strcmp(argv[0], "--y_res") && argc > 1) {
            kos_boot.y_res = strtoul(argv[1], NULL, 0);
            argc -= 2;
            argv += 2;
            continue;
        } else if (!strcmp(argv[0], "--bpp") && argc > 1) {
            kos_boot.bpp = strtoul(argv[1], NULL, 0);
            kos_boot.pitch = kos_boot.x_res * kos_boot.bpp/8;
            argc -= 2;
            argv += 2;
            continue;
        } else if (!strcmp(argv[0], "--pitch") && argc > 1) {
            kos_boot.pitch = strtoul(argv[1], NULL, 0);
            argc -= 2;
            argv += 2;
            continue;
        } else {
            fprintf(ctx->fout, "bad option: %s\n", argv[0]);
            fputs(usage, ctx->fout);
            return;
        }
    }
}

static void
cmd_csleep(struct shell_ctx *ctx, int argc, char **argv) {
    const char *usage = \
        "usage: csleep\n";
    if (argc != 2) {
        fputs(usage, ctx->fout);
        return;
    }
    struct umka_cmd *cmd = umka_cmd_buf;
    struct cmd_sys_csleep_arg *c = &cmd->sys_csleep.arg;
    cmd->type = UMKA_CMD_SYS_CSLEEP;
    c->csec = strtoul(argv[1], NULL, 0);
    shell_run_cmd(ctx);
    atomic_store_explicit(&cmd->status, SHELL_CMD_STATUS_EMPTY,
                          memory_order_release);
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

static void
cmd_wait_for_idle(struct shell_ctx *ctx, int argc, char **argv) {
    (void)argv;
    const char *usage = \
        "usage: wait_for_idle\n";
    if (argc != 1) {
        fputs(usage, ctx->fout);
        return;
    }
    struct umka_cmd *cmd = umka_cmd_buf;
    cmd->type = UMKA_CMD_WAIT_FOR_IDLE;
    shell_run_cmd(ctx);
    atomic_store_explicit(&cmd->status, SHELL_CMD_STATUS_EMPTY,
                          memory_order_release);
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

static void
cmd_wait_for_os_idle(struct shell_ctx *ctx, int argc, char **argv) {
    (void)argv;
    const char *usage = \
        "usage: wait_for_os_idle\n";
    if (argc != 1) {
        fputs(usage, ctx->fout);
        return;
    }
    struct umka_cmd *cmd = umka_cmd_buf;
    cmd->type = UMKA_CMD_WAIT_FOR_OS_IDLE;
    shell_run_cmd(ctx);
    atomic_store_explicit(&cmd->status, SHELL_CMD_STATUS_EMPTY,
                          memory_order_release);
}

static uint32_t
umka_wait_for_window_test(void) {
    appdata_t *app;
    wdata_t *wdata;
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
cmd_wait_for_window(struct shell_ctx *ctx, int argc, char **argv) {
    (void)argv;
    const char *usage = \
        "usage: wait_for_window <window_title>\n";
    if (argc != 2) {
        fputs(usage, ctx->fout);
        return;
    }
    struct umka_cmd *cmd = umka_cmd_buf;
    cmd->type = UMKA_CMD_WAIT_FOR_WINDOW;
    struct cmd_wait_for_window_arg *c = &cmd->wait_for_window.arg;
    c->wnd_title = argv[1];
    shell_run_cmd(ctx);
    atomic_store_explicit(&cmd->status, SHELL_CMD_STATUS_EMPTY,
                          memory_order_release);
}

static void
cmd_i40(struct shell_ctx *ctx, int argc, char **argv) {
    (void)ctx;
    const char *usage = \
        "usage: i40 <eax> [ebx [ecx [edx [esi [edi [ebp]]]]]]...\n"
        "  see '/kernel/docs/sysfuncs.txt' for details\n";
    if (argc < 2 || argc > 8) {
        fputs(usage, ctx->fout);
        return;
    }
    pushad_t regs = {0, 0, 0, 0, 0, 0, 0, 0};
    if (argv[1]) regs.eax = strtoul(argv[1], NULL, 0);
    if (argv[2]) regs.ebx = strtoul(argv[2], NULL, 0);
    if (argv[3]) regs.ecx = strtoul(argv[3], NULL, 0);
    if (argv[4]) regs.edx = strtoul(argv[4], NULL, 0);
    if (argv[5]) regs.esi = strtoul(argv[5], NULL, 0);
    if (argv[6]) regs.edi = strtoul(argv[6], NULL, 0);
    if (argv[7]) regs.ebp = strtoul(argv[7], NULL, 0);
    COVERAGE_ON();
    umka_i40(&regs);
    COVERAGE_OFF();
    fprintf(ctx->fout, "eax = %8.8x  %" PRIu32 "  %" PRIi32 "\n"
           "ebx = %8.8x  %" PRIu32 "  %" PRIi32 "\n",
           regs.eax, regs.eax, (int32_t)regs.eax,
           regs.ebx, regs.ebx, (int32_t)regs.ebx);
}

static void
bytes_to_kmgtpe(uint64_t *bytes, const char **kmg) {
    lldiv_t d;
    *kmg = "B";
    if (*bytes == 0) {
        return;
    }
    d = lldiv(*bytes, 1024);
    if (d.rem != 0) {
        return;
    }
    *bytes = d.quot;
    *kmg = "kiB";
    d = lldiv(*bytes, 1024);
    if (d.rem != 0) {
        return;
    }
    *bytes = d.quot;
    *kmg = "MiB";
    d = lldiv(*bytes, 1024);
    if (d.rem != 0) {
        return;
    }
    *bytes = d.quot;
    *kmg = "GiB";
    d = lldiv(*bytes, 1024);
    if (d.rem != 0) {
        return;
    }
    *bytes = d.quot;
    *kmg = "TiB";
    d = lldiv(*bytes, 1024);
    if (d.rem != 0) {
        return;
    }
    *bytes = d.quot;
    *kmg = "PiB";
    d = lldiv(*bytes, 1024);
    if (d.rem != 0) {
        return;
    }
    *bytes = d.quot;
    *kmg = "EiB";
}

static void
disk_list_partitions(struct shell_ctx *ctx, disk_t *d) {
    uint64_t kmgtpe_count = d->media_info.sector_size * d->media_info.capacity;
    const char *kmgtpe = NULL;
    bytes_to_kmgtpe(&kmgtpe_count, &kmgtpe);
    fprintf(ctx->fout, "/%s: sector_size=%u, capacity=%" PRIu64 " (%" PRIu64
            " %s), num_partitions=%u\n", d->name, d->media_info.sector_size,
            d->media_info.capacity, kmgtpe_count, kmgtpe, d->num_partitions);
    for (size_t i = 0; i < d->num_partitions; i++) {
        partition_t *p = d->partitions[i];
        const char *fsname;
        if (p->fs_user_functions == xfs_user_functions) {
            fsname = "xfs";
        } else if (p->fs_user_functions == ext_user_functions) {
            fsname = "ext";
        } else if (p->fs_user_functions == fat_user_functions) {
            fsname = "fat";
        } else if (p->fs_user_functions == exfat_user_functions) {
            fsname = "exfat";
        } else if (p->fs_user_functions == ntfs_user_functions) {
            fsname = "ntfs";
        } else if (p->fs_user_functions == iso9660_user_functions) {
            fsname = "iso9660";
        } else {
            fsname = "???";
        }
        kmgtpe_count = d->media_info.sector_size * p->first_sector;
        bytes_to_kmgtpe(&kmgtpe_count, &kmgtpe);
        fprintf(ctx->fout, "/%s/%d: fs=%s, start=%" PRIu64 " (%" PRIu64 " %s)",
                d->name, i+1, fsname, p->first_sector, kmgtpe_count, kmgtpe);
        kmgtpe_count = d->media_info.sector_size * p->length;
        bytes_to_kmgtpe(&kmgtpe_count, &kmgtpe);
        fprintf(ctx->fout, ", length=%" PRIu64 " (%" PRIu64 " %s)\n",
                p->length, kmgtpe_count, kmgtpe);
    }
}

static void
cmd_ramdisk_init(struct shell_ctx *ctx, int argc, char **argv) {
    (void)ctx;
    const char *usage = \
        "usage: ramdisk_init <file>\n"
        "  <file>           absolute or relative path\n";
    if (argc != 2) {
        fputs(usage, ctx->fout);
        return;
    }
    const char *fname = argv[1];
    FILE *f = fopen(fname, "rb");
    if (!f) {
        fprintf(ctx->fout, "[!] can't open file '%s': %s\n", fname, strerror(errno));
        return;
    }
    fseek(f, 0, SEEK_END);
    size_t fsize = ftell(f);
    if (fsize > RAMDISK_MAX_LEN) {
        fprintf(ctx->fout, "[!] file '%s' is too big, max size is %d bytes\n",
                fname, RAMDISK_MAX_LEN);
        return;
    }
    rewind(f);
    fread(kos_ramdisk, fsize, 1, f);
    fclose(f);
    COVERAGE_ON();
    void *ramdisk = kos_ramdisk_init();
    COVERAGE_OFF();
    disk_list_partitions(ctx, ramdisk);
}

static void
cmd_disk_add(struct shell_ctx *ctx, int argc, char **argv) {
    const char *usage = \
        "usage: disk_add <file> <name> [option]...\n"
        "  <file>           absolute or relative path\n"
        "  <name>           disk name, e.g. hd0 or rd\n"
        "  -c cache size    size of disk cache in bytes\n";
    if (argc < 3) {
        fputs(usage, ctx->fout);
        return;
    }
    size_t cache_size = 0;
    int adjust_cache_size = 0;
    int opt;
    optparse_init(&ctx->opts, argv);
    const char *file_name = optparse_arg(&ctx->opts);
    const char *disk_name = optparse_arg(&ctx->opts);
    while ((opt = optparse(&ctx->opts, "c:")) != -1) {
        switch (opt) {
        case 'c':
            cache_size = strtoul(ctx->opts.optarg, NULL, 0);
            adjust_cache_size = 1;
            break;
        default:
            fputs(usage, ctx->fout);
            return;
        }
    }

    struct vdisk *umka_disk = vdisk_init(file_name, adjust_cache_size,
                                         cache_size, ctx->io);
    if (umka_disk) {
        COVERAGE_ON();
        disk_t *disk = disk_add(&umka_disk->diskfunc, disk_name, umka_disk, 0);
        COVERAGE_OFF();
        if (disk) {
            COVERAGE_ON();
            disk_media_changed(disk, 1);
            COVERAGE_OFF();
            disk_list_partitions(ctx, disk);
            return;
        }
    }
    fprintf(ctx->fout, "umka: can't add file '%s' as disk '%s'\n", file_name, disk_name);
    return;
}

static void
disk_del_by_name(struct shell_ctx *ctx, const char *name) {
    for(disk_t *d = disk_list.next; d != &disk_list; d = d->next) {
        if (!strcmp(d->name, name)) {
            COVERAGE_ON();
            disk_del(d);
            COVERAGE_OFF();
            return;
        }
    }
    fprintf(ctx->fout, "umka: can't find disk '%s'\n", name);
}

static void
cmd_disk_del(struct shell_ctx *ctx, int argc, char **argv) {
    (void)ctx;
    const char *usage = \
        "usage: disk_del <name>\n"
        "  name             disk name, i.e. rd or hd0\n";
    if (argc != 2) {
        fputs(usage, ctx->fout);
        return;
    }
    const char *name = argv[1];
    disk_del_by_name(ctx, name);
    return;
}

static void
cmd_pwd(struct shell_ctx *ctx, int argc, char **argv) {
    (void)ctx;
    const char *usage = \
        "usage: pwd\n";
    if (argc != 1) {
        fputs(usage, ctx->fout);
        return;
    }
    (void)argv;
    bool quoted = false;
    const char *quote = quoted ? "'" : "";
    COVERAGE_ON();
    umka_sys_get_cwd(cur_dir, PATH_MAX);
    COVERAGE_OFF();
    fprintf(ctx->fout, "%s%s%s\n", quote, cur_dir, quote);
}

static void
cmd_set_pixel(struct shell_ctx *ctx, int argc, char **argv) {
    (void)ctx;
    const char *usage = \
        "usage: set_pixel <x> <y> <color> [-i]\n"
        "  x                x window coordinate\n"
        "  y                y window coordinate\n"
        "  color            argb in hex\n"
        "  -i               inverted color\n";
    if (argc < 4) {
        fputs(usage, ctx->fout);
        return;
    }
    size_t x = strtoul(argv[1], NULL, 0);
    size_t y = strtoul(argv[2], NULL, 0);
    uint32_t color = strtoul(argv[3], NULL, 16);
    int invert = (argc == 5) && !strcmp(argv[4], "-i");
    COVERAGE_ON();
    umka_sys_set_pixel(x, y, color, invert);
    COVERAGE_OFF();
}

static void
cmd_write_text(struct shell_ctx *ctx, int argc, char **argv) {
    (void)ctx;
    const char *usage = \
        "usage: write_text <x> <y> <color> <string> <asciiz> <fill_bg>"
            " <font_and_enc> <draw_to_buf> <scale_factor> <length>"
            " <bg_color_or_buf>\n"
        "  x                x window coordinate\n"
        "  y                y window coordinate\n"
        "  color            argb in hex\n"
        "  string           escape spaces\n"
        "  asciiz           1 if the string is zero-terminated\n"
        "  fill_bg          fill text background with specified color\n"
        "  font_and_enc     font size and string encoding\n"
        "  draw_to_buf      draw to the buffer pointed to by the next param\n"
        "  length           length of the string if it is non-asciiz\n"
        "  bg_color_or_buf  argb or pointer\n";
    if (argc != 12) {
        fputs(usage, ctx->fout);
        return;
    }
    size_t x = strtoul(argv[1], NULL, 0);
    size_t y = strtoul(argv[2], NULL, 0);
    uint32_t color = strtoul(argv[3], NULL, 16);
    const char *string = argv[4];
    int asciiz = strtoul(argv[5], NULL, 0);
    int fill_background = strtoul(argv[6], NULL, 0);
    int font_and_encoding = strtoul(argv[7], NULL, 0);
    int draw_to_buffer = strtoul(argv[8], NULL, 0);
    int scale_factor = strtoul(argv[9], NULL, 0);
    int length = strtoul(argv[10], NULL, 0);
    int background_color_or_buffer = strtoul(argv[11], NULL, 0);
    COVERAGE_ON();
    umka_sys_write_text(x, y, color, asciiz, fill_background, font_and_encoding,
                        draw_to_buffer, scale_factor, string, length,
                        background_color_or_buffer);
    COVERAGE_OFF();
}

static void
cmd_get_key(struct shell_ctx *ctx, int argc, char **argv) {
    (void)ctx;
    (void)argv;
    const char *usage = \
        "usage: get_key\n";
    if (argc > 1) {
        fputs(usage, ctx->fout);
        return;
    }
    fprintf(ctx->fout, "0x%8.8" PRIx32 "\n", umka_get_key());
}

static void
cmd_dump_key_buff(struct shell_ctx *ctx, int argc, char **argv) {
    (void)ctx;
    const char *usage = \
        "usage: dump_key_buff [count]\n"
        "  count            how many items to dump (all by default)\n";
    if (argc > 2) {
        fputs(usage, ctx->fout);
        return;
    }
    int count = INT_MAX;
    if (argc > 1) {
        count = strtol(argv[1], NULL, 0);
    }
    for (int i = 0; i < count && i < kos_key_count; i++) {
        fprintf(ctx->fout, "%3i 0x%2.2x 0x%2.2x\n", i, kos_key_buff[i],
               kos_key_buff[120+2+i]);
    }
}

static void
cmd_dump_win_stack(struct shell_ctx *ctx, int argc, char **argv) {
    (void)ctx;
    const char *usage = \
        "usage: dump_win_stack [count]\n"
        "  count            how many items to dump\n";
    if (argc > 2) {
        fputs(usage, ctx->fout);
        return;
    }
    int depth = 5;
    if (argc > 1) {
        depth = strtol(argv[1], NULL, 0);
    }
    for (int i = 0; i < depth; i++) {
        fprintf(ctx->fout, "%3i: %3u\n", i, kos_win_stack[i]);
    }
}

static void
cmd_dump_win_pos(struct shell_ctx *ctx, int argc, char **argv) {
    (void)ctx;
    const char *usage = \
        "usage: dump_win_pos [count]\n"
        "  count            how many items to dump\n";
    if (argc < 1) {
        fputs(usage, ctx->fout);
        return;
    }
    int depth = 5;
    if (argc > 1) {
        depth = strtol(argv[1], NULL, 0);
    }
    for (int i = 0; i < depth; i++) {
        fprintf(ctx->fout, "%3i: %3u\n", i, kos_win_pos[i]);
    }
}

static void
cmd_dump_win_map(struct shell_ctx *ctx, int argc, char **argv) {
    (void)ctx;
    (void)argv;
// TODO: area
    const char *usage = \
        "usage: dump_win_map\n";
    if (argc < 0) {
        fputs(usage, ctx->fout);
        return;
    }
    for (size_t y = 0; y < kos_display.height; y++) {
        for (size_t x = 0; x < kos_display.width; x++) {
            fprintf(ctx->fout, "%c",
                    kos_display.win_map[y * kos_display.width + x] + '0');
        }
        fprintf(ctx->fout, "\n");
    }
}

static void
cmd_dump_wdata(struct shell_ctx *ctx, int argc, char **argv) {
    (void)ctx;
    const char *usage = \
        "usage: dump_wdata <index>\n"
        "  index            index into wdata array to dump\n"
        "  -p               print fields that are pointers\n";
    if (argc < 2) {
        fputs(usage, ctx->fout);
        return;
    }
    int show_pointers = 0;
    int idx = strtol(argv[1], NULL, 0);
    if (argc > 2 && !strcmp(argv[2], "-p")) {
        show_pointers = 1;
    }
    wdata_t *w = kos_window_data + idx;

    fprintf(ctx->fout, "captionEncoding: %u\n", w->caption_encoding);
    fprintf(ctx->fout, "caption: %s\n", w->caption);
    fprintf(ctx->fout, "clientbox (ltwh): %u %u %u %u\n", w->clientbox.left,
           w->clientbox.top, w->clientbox.width, w->clientbox.height);
    fprintf(ctx->fout, "draw_bgr_x: %u\n", w->draw_bgr_x);
    fprintf(ctx->fout, "draw_bgr_y: %u\n", w->draw_bgr_y);
    fprintf(ctx->fout, "thread: %u\n", (uintptr_t)(w->thread - kos_slot_base));
    if (show_pointers) {
        fprintf(ctx->fout, "thread: %p\n", (void*)w->thread);
        fprintf(ctx->fout, "cursor: %p\n", (void*)w->cursor);
    }
}

static void
cmd_dump_appdata(struct shell_ctx *ctx, int argc, char **argv) {
    (void)ctx;
    const char *usage = \
        "usage: dump_appdata <index> [-p]\n"
        "  index            index into appdata array to dump\n"
        "  -p               print fields that are pointers\n";
    if (argc < 2) {
        fputs(usage, ctx->fout);
        return;
    }
    int show_pointers = 0;
    int idx = strtol(argv[1], NULL, 0);
    if (argc > 2 && !strcmp(argv[2], "-p")) {
        show_pointers = 1;
    }
    appdata_t *a = kos_slot_base + idx;
    fprintf(ctx->fout, "app_name: %s\n", a->app_name);
    if (show_pointers) {
        fprintf(ctx->fout, "process: %p\n", (void*)a->process);
        fprintf(ctx->fout, "fpu_state: %p\n", (void*)a->fpu_state);
        fprintf(ctx->fout, "exc_handler: %p\n", (void*)a->exc_handler);
    }
    fprintf(ctx->fout, "except_mask: %" PRIx32 "\n", a->except_mask);
    if (show_pointers) {
        fprintf(ctx->fout, "pl0_stack: %p\n", (void*)a->pl0_stack);
        fprintf(ctx->fout, "fd_ev: %p\n", (void*)a->fd_ev);
        fprintf(ctx->fout, "bk_ev: %p\n", (void*)a->bk_ev);
        fprintf(ctx->fout, "fd_obj: %p\n", (void*)a->fd_obj);
        fprintf(ctx->fout, "bk_obj: %p\n", (void*)a->bk_obj);
        fprintf(ctx->fout, "saved_esp: %p\n", (void*)a->saved_esp);
    }
    fprintf(ctx->fout, "dbg_state: %u\n", a->dbg_state);
    fprintf(ctx->fout, "cur_dir: %s\n", a->cur_dir);
    fprintf(ctx->fout, "event_mask: %" PRIx32 "\n", a->event_mask);
    fprintf(ctx->fout, "tid: %" PRId32 "\n", a->tid);
    fprintf(ctx->fout, "state: 0x%" PRIx8 "\n", a->state);
    fprintf(ctx->fout, "wnd_number: %" PRIu8 "\n", a->wnd_number);
    fprintf(ctx->fout, "terminate_protection: %u\n", a->terminate_protection);
    fprintf(ctx->fout, "keyboard_mode: %u\n", a->keyboard_mode);
    fprintf(ctx->fout, "exec_params: %s\n", a->exec_params);
    fprintf(ctx->fout, "priority: %u\n", a->priority);

    fprintf(ctx->fout, "in_schedule: prev");
    if (show_pointers) {
        fprintf(ctx->fout, " %p", (void*)a->in_schedule.prev);
    }
    fprintf(ctx->fout, " (%u), next", (appdata_t*)a->in_schedule.prev - kos_slot_base);
    if (show_pointers) {
        fprintf(ctx->fout, " %p", (void*)a->in_schedule.next);
    }
    fprintf(ctx->fout, " (%u)\n", (appdata_t*)a->in_schedule.next - kos_slot_base);
}

static void
cmd_switch_to_thread(struct shell_ctx *ctx, int argc, char **argv) {
    (void)ctx;
    const char *usage = \
        "usage: switch_to_thread <tid>\n"
        "  <tid>          thread id to switch to\n";
    if (argc != 2) {
        fputs(usage, ctx->fout);
        return;
    }
    uint8_t tid = strtoul(argv[1], NULL, 0);
    kos_current_slot_idx = tid;
    kos_current_slot = kos_slot_base + tid;
}

static void
cmd_get(struct shell_ctx *ctx, int argc, char **argv) {
    (void)ctx;
    const char *usage = \
        "usage: get <var>\n"
        "  <var>          variable to get\n";
    if (argc != 2) {
        fputs(usage, ctx->fout);
        return;
    }
    const char *var = argv[1];
    if (!strcmp(var, "redraw_background")) {
        fprintf(ctx->fout, "%i\n", kos_redraw_background);
    } else if (!strcmp(var, "key_count")) {
        fprintf(ctx->fout, "%i\n", kos_key_count);
    } else if (!strcmp(var, "syslang")) {
        fprintf(ctx->fout, "%i\n", kos_syslang);
    } else if (!strcmp(var, "keyboard")) {
        fprintf(ctx->fout, "%i\n", kos_keyboard);
    } else if (!strcmp(var, "keyboard_mode")) {
        fprintf(ctx->fout, "%i\n", kos_keyboard_mode);
    } else {
        fprintf(ctx->fout, "no such variable: %s\n", var);
        fputs(usage, ctx->fout);
        return;
    }
}

static void
cmd_set(struct shell_ctx *ctx, int argc, char **argv) {
    (void)ctx;
    const char *usage = \
        "usage: set <var> <value>\n"
        "  <var>          variable to set\n"
        "  <value>        decimal or hex value\n";
    if (argc != 3) {
        fputs(usage, ctx->fout);
        return;
    }
    const char *var = argv[1];
    const char *val_str = argv[2];
    char *endptr;
    ssize_t value = strtol(val_str, &endptr, 0);
    if (*endptr != '\0') {
        fprintf(ctx->fout, "integer required: %s\n", val_str);
        return;
    }
    if (!strcmp(var, "redraw_background")) {
        kos_redraw_background = value;
    } else if (!strcmp(var, "syslang")) {
        kos_syslang = value;
    } else if (!strcmp(var, "keyboard")) {
        kos_keyboard = value;
    } else if (!strcmp(var, "keyboard_mode")) {
        kos_keyboard_mode = value;
    } else {
        fprintf(ctx->fout, "bad option: %s\n", argv[1]);
        fputs(usage, ctx->fout);
        return;
    }
}

static void
cmd_new_sys_thread(struct shell_ctx *ctx, int argc, char **argv) {
    (void)ctx;
    (void)argv;
// FIXME
    const char *usage = \
        "usage: new_sys_thread\n";
    if (!argc) {
        fputs(usage, ctx->fout);
        return;
    }
    size_t tid = kos_new_sys_threads(0, NULL, NULL);
    fprintf(ctx->fout, "tid: %u\n", tid);
}

static void
cmd_mouse_move(struct shell_ctx *ctx, int argc, char **argv) {
    (void)ctx;
    const char *usage = \
        "usage: mouse_move [-l] [-m] [-r] [-x {+|-|=}<value>]"
            " [-y {+|-|=}<value>] [-h {+|-}<value>] [-v {+|-}<value>]\n"
        "  -l             left button is held\n"
        "  -m             middle button is held\n"
        "  -r             right button is held\n"
        "  -x             increase, decrease or set x coordinate\n"
        "  -y             increase, decrease or set y coordinate\n"
        "  -h             scroll horizontally\n"
        "  -v             scroll vertically\n";
    if (!argc) {
        fputs(usage, ctx->fout);
        return;
    }

    int lbheld = 0, mbheld = 0, rbheld = 0, xabs = 0, yabs = 0;
    int32_t xmoving = 0, ymoving = 0, hscroll = 0, vscroll = 0;
    int opt;
    optparse_init(&ctx->opts, argv);
    while ((opt = optparse(&ctx->opts, "lmrx:y:h:v:")) != -1) {
        switch (opt) {
        case 'l':
            lbheld = 1;
            break;
        case 'm':
            mbheld = 1;
            break;
        case 'r':
            rbheld = 1;
            break;
        case 'x':
            switch (*ctx->opts.optarg++) {
            case '=':
                xabs = 1;
                __attribute__ ((fallthrough));
            case '+':
                xmoving = strtol(ctx->opts.optarg, NULL, 0);
                break;
            case '-':
                xmoving = -strtol(ctx->opts.optarg, NULL, 0);
                break;
            default:
                fputs(usage, ctx->fout);
                return;
            }
            break;
        case 'y':
            switch (*ctx->opts.optarg++) {
            case '=':
                yabs = 1;
                __attribute__ ((fallthrough));
            case '+':
                ymoving = strtol(ctx->opts.optarg, NULL, 0);
                break;
            case '-':
                ymoving = -strtol(ctx->opts.optarg, NULL, 0);
                break;
            default:
                fputs(usage, ctx->fout);
                return;
            }
            break;
        case 'h':
            if ((ctx->opts.optarg[0] != '+') && (ctx->opts.optarg[0] != '-')) {
                fputs(usage, ctx->fout);
                return;
            }
            hscroll = strtol(ctx->opts.optarg, NULL, 0);
            break;
        case 'v':
            if ((ctx->opts.optarg[0] != '+') && (ctx->opts.optarg[0] != '-')) {
                fputs(usage, ctx->fout);
                return;
            }
            vscroll = strtol(ctx->opts.optarg, NULL, 0);
            break;
        default:
            fputs(usage, ctx->fout);
            return;
        }
    }
    uint32_t btn_state = lbheld + (rbheld << 1) + (mbheld << 2) + (yabs << 30)
                         + (xabs << 31);
    struct umka_cmd *cmd = shell_get_cmd(ctx);
    cmd->type = UMKA_CMD_SET_MOUSE_DATA;
    struct cmd_set_mouse_data_arg *c = &cmd->set_mouse_data.arg;
    c->btn_state = btn_state;
    c->xmoving = xmoving;
    c->ymoving = ymoving;
    c->vscroll = vscroll;
    c->hscroll = hscroll;
    shell_run_cmd(ctx);
    shell_clear_cmd(cmd);
}

static void
cmd_process_info(struct shell_ctx *ctx, int argc, char **argv) {
    const char *usage = \
        "usage: process_info <pid>\n"
        "  pid              process id to dump, -1 for self\n";
    if (argc != 2) {
        fputs(usage, ctx->fout);
        return;
    }
    process_information_t info;
    ssize_t pid;
    shell_parse_sint(ctx, &pid, argv[1]);
    COVERAGE_ON();
    umka_sys_process_info(pid, &info);
    COVERAGE_OFF();
    fprintf(ctx->fout, "cpu_usage: %u\n", info.cpu_usage);
    fprintf(ctx->fout, "window_stack_position: %u\n", info.window_stack_position);
    fprintf(ctx->fout, "window_stack_value: %u\n", info.window_stack_value);
    fprintf(ctx->fout, "process_name: %s\n", info.process_name);
    fprintf(ctx->fout, "memory_start: 0x%.8" PRIx32 "\n", info.memory_start);
    fprintf(ctx->fout, "used_memory: %u (0x%x)\n", info.used_memory,
           info.used_memory);
    fprintf(ctx->fout, "pid: %u\n", info.pid);
    fprintf(ctx->fout, "box: %u %u %u %u\n", info.box.left, info.box.top,
           info.box.width, info.box.height);
    fprintf(ctx->fout, "slot_state: %u\n", info.slot_state);
    fprintf(ctx->fout, "client_box: %u %u %u %u\n", info.client_box.left,
           info.client_box.top, info.client_box.width, info.client_box.height);
    fprintf(ctx->fout, "wnd_state: 0x%.2" PRIx8 "\n", info.wnd_state);
}

static void
cmd_check_for_event(struct shell_ctx *ctx, int argc, char **argv) {
    (void)ctx;
    (void)argv;
    const char *usage = \
        "usage: check_for_event\n";
    if (argc != 1) {
        fputs(usage, ctx->fout);
        return;
    }
    COVERAGE_ON();
    uint32_t event = umka_sys_check_for_event();
    COVERAGE_OFF();
    fprintf(ctx->fout, "%" PRIu32 "\n", event);
}

static void
cmd_display_number(struct shell_ctx *ctx, int argc, char **argv) {
    (void)ctx;
    const char *usage = \
        "usage: display_number <is_pointer> <base> <num_digits> <is_qword>"
            " <show_lead_zeros> <num_or_ptr> <x> <y> <color> <fill_bg> <font>"
            " <draw_to_buf> <scale_factor> <bg_color_or_buf>\n"
        "  is_pointer       if num_or_ptr argument is a pointer\n"
        "  base             0 - dec, 1 - hex, 2 - bin\n"
        "  num_digits       how many digits to print\n"
        "  is_qword         if 1, is_pointer = 1 and num_or_ptr is pointer\n"
        "  show_lead_zeros  0/1\n"
        "  num_or_ptr       number itself or a pointer to it\n"
        "  x                x window coord\n"
        "  y                y window coord\n"
        "  color            argb in hex\n"
        "  fill_bg          0/1\n"
        "  font             0 = 6x9, 1 = 8x16\n"
        "  draw_to_buf      0/1\n"
        "  scale_factor     0 = x1, ..., 7 = x8\n"
        "  bg_color_or_buf  depending on flags fill_bg and draw_to_buf\n";
    if (argc != 15) {
        fputs(usage, ctx->fout);
        return;
    }
    int is_pointer = strtoul(argv[1], NULL, 0);
    int base = strtoul(argv[2], NULL, 0);
    if (base == 10) base = 0;
    else if (base == 16) base = 1;
    else if (base == 2) base = 2;
    else base = 0;
    size_t digits_to_display = strtoul(argv[3], NULL, 0);
    int is_qword = strtoul(argv[4], NULL, 0);
    int show_leading_zeros = strtoul(argv[5], NULL, 0);
    uintptr_t number_or_pointer = strtoul(argv[6], NULL, 0);
    size_t x = strtoul(argv[7], NULL, 0);
    size_t y = strtoul(argv[8], NULL, 0);
    uint32_t color = strtoul(argv[9], NULL, 16);
    int fill_background = strtoul(argv[10], NULL, 0);
    int font = strtoul(argv[11], NULL, 0);
    int draw_to_buffer = strtoul(argv[12], NULL, 0);
    int scale_factor = strtoul(argv[13], NULL, 0);
    uintptr_t background_color_or_buffer = strtoul(argv[14], NULL, 16);
    COVERAGE_ON();
    umka_sys_display_number(is_pointer, base, digits_to_display, is_qword,
                            show_leading_zeros, number_or_pointer, x, y, color,
                            fill_background, font, draw_to_buffer, scale_factor,
                            background_color_or_buffer);
    COVERAGE_OFF();
}

static void
cmd_set_window_colors(struct shell_ctx *ctx, int argc, char **argv) {
    (void)ctx;
    const char *usage = \
        "usage: set_window_colors <frame> <grab> <work_3d_dark> <work_3d_light>"
            " <grab_text> <work> <work_button> <work_button_text> <work_text>"
            " <work_graph>\n"
        "  *                all colors are in hex\n";
    if (argc != (1 + sizeof(system_colors_t)/4)) {
        fputs(usage, ctx->fout);
        return;
    }
    system_colors_t colors;
    colors.frame            = strtoul(argv[1], NULL, 16);
    colors.grab             = strtoul(argv[2], NULL, 16);
    colors.work_3d_dark     = strtoul(argv[3], NULL, 16);
    colors.work_3d_light    = strtoul(argv[4], NULL, 16);
    colors.grab_text        = strtoul(argv[5], NULL, 16);
    colors.work             = strtoul(argv[6], NULL, 16);
    colors.work_button      = strtoul(argv[7], NULL, 16);
    colors.work_button_text = strtoul(argv[8], NULL, 16);
    colors.work_text        = strtoul(argv[9], NULL, 16);
    colors.work_graph       = strtoul(argv[10], NULL, 16);
    COVERAGE_ON();
    umka_sys_set_window_colors(&colors);
    COVERAGE_OFF();
}

static void
cmd_get_window_colors(struct shell_ctx *ctx, int argc, char **argv) {
    (void)ctx;
    (void)argv;
    const char *usage = \
        "usage: get_window_colors\n";
    if (argc != 1) {
        fputs(usage, ctx->fout);
        return;
    }
    system_colors_t colors;
    memset(&colors, 0xaa, sizeof(colors));
    COVERAGE_ON();
    umka_sys_get_window_colors(&colors);
    COVERAGE_OFF();
    fprintf(ctx->fout, "0x%.8" PRIx32 " frame\n", colors.frame);
    fprintf(ctx->fout, "0x%.8" PRIx32 " grab\n", colors.grab);
    fprintf(ctx->fout, "0x%.8" PRIx32 " work_3d_dark\n", colors.work_3d_dark);
    fprintf(ctx->fout, "0x%.8" PRIx32 " work_3d_light\n", colors.work_3d_light);
    fprintf(ctx->fout, "0x%.8" PRIx32 " grab_text\n", colors.grab_text);
    fprintf(ctx->fout, "0x%.8" PRIx32 " work\n", colors.work);
    fprintf(ctx->fout, "0x%.8" PRIx32 " work_button\n", colors.work_button);
    fprintf(ctx->fout, "0x%.8" PRIx32 " work_button_text\n",
           colors.work_button_text);
    fprintf(ctx->fout, "0x%.8" PRIx32 " work_text\n", colors.work_text);
    fprintf(ctx->fout, "0x%.8" PRIx32 " work_graph\n", colors.work_graph);
}

static void
cmd_get_skin_height(struct shell_ctx *ctx, int argc, char **argv) {
    (void)ctx;
    (void)argv;
    const char *usage = \
        "usage: get_skin_height\n";
    if (argc != 1) {
        fputs(usage, ctx->fout);
        return;
    }
    COVERAGE_ON();
    uint32_t skin_height = umka_sys_get_skin_height();
    COVERAGE_OFF();
    fprintf(ctx->fout, "%" PRIu32 "\n", skin_height);
}

static void
cmd_get_screen_area(struct shell_ctx *ctx, int argc, char **argv) {
    (void)ctx;
    (void)argv;
    const char *usage = \
        "usage: get_screen_area\n";
    if (argc != 1) {
        fputs(usage, ctx->fout);
        return;
    }
    rect_t wa;
    COVERAGE_ON();
    umka_sys_get_screen_area(&wa);
    COVERAGE_OFF();
    fprintf(ctx->fout, "%" PRIu32 " left\n", wa.left);
    fprintf(ctx->fout, "%" PRIu32 " top\n", wa.top);
    fprintf(ctx->fout, "%" PRIu32 " right\n", wa.right);
    fprintf(ctx->fout, "%" PRIu32 " bottom\n", wa.bottom);
}

static void
cmd_set_screen_area(struct shell_ctx *ctx, int argc, char **argv) {
    (void)ctx;
    const char *usage = \
        "usage: set_screen_area <left> <top> <right> <bottom>\n"
        "  left             left x coord\n"
        "  top              top y coord\n"
        "  right            right x coord (not length!)\n"
        "  bottom           bottom y coord\n";
    if (argc != 5) {
        fputs(usage, ctx->fout);
        return;
    }
    rect_t wa;
    wa.left   = strtoul(argv[1], NULL, 0);
    wa.top    = strtoul(argv[2], NULL, 0);
    wa.right  = strtoul(argv[3], NULL, 0);
    wa.bottom = strtoul(argv[4], NULL, 0);
    COVERAGE_ON();
    umka_sys_set_screen_area(&wa);
    COVERAGE_OFF();
}

static void
cmd_get_skin_margins(struct shell_ctx *ctx, int argc, char **argv) {
    (void)ctx;
    (void)argv;
    const char *usage = \
        "usage: get_skin_margins\n";
    if (argc != 1) {
        fputs(usage, ctx->fout);
        return;
    }
    rect_t wa;
    COVERAGE_ON();
    umka_sys_get_skin_margins(&wa);
    COVERAGE_OFF();
    fprintf(ctx->fout, "%" PRIu32 " left\n", wa.left);
    fprintf(ctx->fout, "%" PRIu32 " top\n", wa.top);
    fprintf(ctx->fout, "%" PRIu32 " right\n", wa.right);
    fprintf(ctx->fout, "%" PRIu32 " bottom\n", wa.bottom);
}

static void
cmd_set_button_style(struct shell_ctx *ctx, int argc, char **argv) {
    (void)ctx;
    const char *usage = \
        "usage: set_button_style <style>\n"
        "  style            0 - flat, 1 - 3d\n";
    if (argc != 2) {
        fputs(usage, ctx->fout);
        return;
    }
    uint32_t style = strtoul(argv[1], NULL, 0);
    COVERAGE_ON();
    umka_sys_set_button_style(style);
    COVERAGE_OFF();
}

static void
cmd_set_skin(struct shell_ctx *ctx, int argc, char **argv) {
    (void)ctx;
    const char *usage = \
        "usage: set_skin <path>\n"
        "  path             i.e. /rd/1/DEFAULT.SKN\n";
    if (argc != 2) {
        fputs(usage, ctx->fout);
        return;
    }
    const char *path = argv[1];
    COVERAGE_ON();
    int32_t status = umka_sys_set_skin(path);
    COVERAGE_OFF();
    fprintf(ctx->fout, "status: %" PRIi32 "\n", status);
}

char *lang_id_map[] = {"en", "fi", "ge", "ru", "fr", "et", "ua", "it", "be",
                       "sp", "ca"};

static void
cmd_get_keyboard_layout(struct shell_ctx *ctx, int argc, char **argv) {
    (void)ctx;
    const char *usage = \
        "usage: get_keyboard_layout <-t type> [-f file]\n"
        "  -t type          layout type: 1 - normal, 2 - shift, 3 - alt\n"
        "  -f file          file name to save layout to\n";
    if (argc < 3) {
        fputs(usage, ctx->fout);
        return;
    }
    int type = -1;
    if (!strcmp(argv[1], "-t")) {
        const char *type_str = argv[2];
        char *endptr;
        type = strtol(type_str, &endptr, 0);
        if (*endptr != '\0') {
            if (!strcmp(type_str, "normal")) {
                type = 1;
            } else if (!strcmp(type_str, "shift")) {
                type = 2;
            } else if (!strcmp(type_str, "alt")) {
                type = 3;
            } else {
                fprintf(ctx->fout, "no such layout type: %s\n", type_str);
                fputs(usage, ctx->fout);
                return;
            }
        }
    }
    uint8_t layout[KOS_LAYOUT_SIZE];
    COVERAGE_ON();
    umka_sys_get_keyboard_layout(type, layout);
    COVERAGE_OFF();
    if (argc == 3) {
#define NUM_COLS 16 // produces nice output, 80 chars
        for (size_t row = 0; row < KOS_LAYOUT_SIZE/NUM_COLS; row++) {
            for (size_t col = 0; col < NUM_COLS; col++) {
                fprintf(ctx->fout, " 0x%2.2x", layout[row*NUM_COLS+col]);
            }
            fprintf(ctx->fout, "\n");
        }
#undef COLS
    } else if (argc == 5 && !strcmp(argv[3], "-f")) {
        const char *fname = argv[4];
        FILE *f = fopen(fname, "wb");
        if (!f) {
            fprintf(ctx->fout, "can't open file for writing: %s\n", fname);
            fputs(usage, ctx->fout);
            return;
        }
        size_t nread = fwrite(layout, 1, KOS_LAYOUT_SIZE, f);
        fclose(f);
        if (nread != KOS_LAYOUT_SIZE) {
            fprintf(ctx->fout, "can't write %i bytes of layout, only %i\n",
                   KOS_LAYOUT_SIZE, nread);
            fputs(usage, ctx->fout);
            return;
        }
    } else {
        fputs(usage, ctx->fout);
        return;
    }
}

static void
cmd_set_keyboard_layout(struct shell_ctx *ctx, int argc, char **argv) {
    (void)ctx;
    const char *usage = \
        "usage: set_keyboard_layout <-t type> <-f file|code{128}>\n"
        "  -t type          layout type: 1 - normal, 2 - shift, 3 - alt\n"
        "  -f file          file name to read layout from\n"
        "  code             i-th ASCII code for the scancode i\n";
    if (argc != 5 && argc != 131) {
        fputs(usage, ctx->fout);
        return;
    }
    int type;
    if (!strcmp(argv[1], "-t")) {
        const char *type_str = argv[2];
        char *endptr;
        type = strtol(type_str, &endptr, 0);
        if (*endptr != '\0') {
            if (!strcmp(type_str, "normal")) {
                type = 1;
            } else if (!strcmp(type_str, "shift")) {
                type = 2;
            } else if (!strcmp(type_str, "alt")) {
                type = 3;
            } else {
                fprintf(ctx->fout, "no such layout type: %s\n", type_str);
                fputs(usage, ctx->fout);
                return;
            }
        }
    }
    uint8_t layout[KOS_LAYOUT_SIZE];
    if (!strcmp(argv[3], "-f") && argc == 5) {
        const char *fname = argv[4];
        FILE *f = fopen(fname, "rb");
        if (!f) {
            fprintf(ctx->fout, "can't open file: %s\n", fname);
            fputs(usage, ctx->fout);
            return;
        }
        size_t nread = fread(layout, 1, KOS_LAYOUT_SIZE, f);
        fclose(f);
        if (nread != KOS_LAYOUT_SIZE) {
            fprintf(ctx->fout, "can't read %i bytes of layout, only %i\n",
                   KOS_LAYOUT_SIZE, nread);
            fputs(usage, ctx->fout);
            return;
        }
    } else {
        char *endptr;
        char **ascii_codes = argv + 4;
        for (size_t i = 0; i < KOS_LAYOUT_SIZE; i++) {
            long code = strtol(ascii_codes[i], &endptr, 0);
            if (*endptr != '\0' || code > 0xff) {
                fprintf(ctx->fout, "bad number: %s\n", ascii_codes[i]);
                fputs(usage, ctx->fout);
                return;
            }
            layout[i] = code;
        }
    }
    COVERAGE_ON();
    int status = umka_sys_set_keyboard_layout(type, layout);
    COVERAGE_OFF();
    fprintf(ctx->fout, "status: %i\n", status);
}

static void
cmd_get_keyboard_lang(struct shell_ctx *ctx, int argc, char **argv) {
    (void)ctx;
    (void)argv;
    const char *usage = \
        "usage: get_keyboard_lang\n";
    if (argc != 1) {
        fputs(usage, ctx->fout);
        return;
    }
    COVERAGE_ON();
    int lang = umka_sys_get_keyboard_lang();
    COVERAGE_OFF();
    if (lang >= KOS_LANG_FIRST && lang <= KOS_LANG_LAST) {
        fprintf(ctx->fout, "%i - %s\n", lang, lang_id_map[lang - KOS_LANG_FIRST]);
    } else {
        fprintf(ctx->fout, "invalid lang: %i\n", lang);
    }
    shell_var_add_sint(ctx, lang);
}

static void
cmd_get_system_lang(struct shell_ctx *ctx, int argc, char **argv) {
    (void)ctx;
    (void)argv;
    const char *usage = \
        "usage: get_system_lang\n";
    if (argc != 1) {
        fputs(usage, ctx->fout);
        return;
    }
    COVERAGE_ON();
    int lang = umka_sys_get_system_lang();
    COVERAGE_OFF();
    if (lang >= KOS_LANG_FIRST && lang <= KOS_LANG_LAST) {
        fprintf(ctx->fout, "%i - %s\n", lang, lang_id_map[lang - KOS_LANG_FIRST]);
    } else {
        fprintf(ctx->fout, "invalid lang: %i\n", lang);
    }
    shell_var_add_uint(ctx, lang);
}

static void
cmd_set_keyboard_lang(struct shell_ctx *ctx, int argc, char **argv) {
    (void)ctx;
    const char *usage = \
        "usage: set_keyboard_lang <lang_id>\n"
        "  lang_id          number or a two-digit code:\n"
        "                   1 - en, 2 - fi, 3 - ge, 4 - ru, 5 - fr, 6 - et,\n"
        "                   7 - ua, 8 - it, 9 - be, 10 - sp, 11 - ca\n";
    if (argc != 2) {
        fputs(usage, ctx->fout);
        return;
    }
    char *endptr;
    const char *lang_str = argv[1];
    int lang = strtol(lang_str, &endptr, 0);
    if (*endptr != '\0') {
        for (lang = 0; lang < KOS_LANG_LAST; lang++) {
            if (!strcmp(lang_str, lang_id_map[lang])) {
                break;
            }
        }
        if (lang == KOS_LANG_LAST) {
            fprintf(ctx->fout, "no such lang: %s\n", lang_str);
            fputs(usage, ctx->fout);
            return;
        }
        lang += KOS_LANG_FIRST;
    }
    COVERAGE_ON();
    umka_sys_set_keyboard_lang(lang);
    COVERAGE_OFF();
}

static void
cmd_set_system_lang(struct shell_ctx *ctx, int argc, char **argv) {
    (void)ctx;
    const char *usage = \
        "usage: set_system_lang <lang_id>\n"
        "  lang_id          number or a two-digit code:\n"
        "                   1 - en, 2 - fi, 3 - ge, 4 - ru, 5 - fr, 6 - et,\n"
        "                   7 - ua, 8 - it, 9 - be, 10 - sp, 11 - ca\n";
    if (argc != 2) {
        fputs(usage, ctx->fout);
        return;
    }
    char *endptr;
    const char *lang_str = argv[1];
    int lang = strtol(lang_str, &endptr, 0);
    if (*endptr != '\0') {
        for (lang = 0; lang < KOS_LANG_LAST; lang++) {
            if (!strcmp(lang_str, lang_id_map[lang])) {
                break;
            }
        }
        if (lang == KOS_LANG_LAST) {
            fprintf(ctx->fout, "no such lang: %s\n", lang_str);
            fputs(usage, ctx->fout);
            return;
        }
        lang += KOS_LANG_FIRST;  // not zero-based
    }
    COVERAGE_ON();
    umka_sys_set_system_lang(lang);
    COVERAGE_OFF();
}

static void
cmd_get_keyboard_mode(struct shell_ctx *ctx, int argc, char **argv) {
    (void)ctx;
    (void)argv;
    const char *usage = \
        "usage: get_keyboard_mode\n";
    if (argc != 1) {
        fputs(usage, ctx->fout);
        return;
    }
    const char *names[] = {"ASCII", "scancodes"};
    COVERAGE_ON();
    int type = umka_sys_get_keyboard_mode();
    COVERAGE_OFF();
    fprintf(ctx->fout, "keyboard_mode: %i - %s\n", type,
           type < 2 ? names[type] : "invalid!");
}

static void
cmd_set_keyboard_mode(struct shell_ctx *ctx, int argc, char **argv) {
    (void)ctx;
    const char *usage = \
        "usage: set_keyboard_mode <mode>\n"
        "  mode             0 - ASCII, 1 - scancodes\n";
    if (argc != 2) {
        fputs(usage, ctx->fout);
        return;
    }
    int type = strtol(argv[1], NULL, 0);
    COVERAGE_ON();
    umka_sys_set_keyboard_mode(type);
    COVERAGE_OFF();
}

static void
cmd_get_font_smoothing(struct shell_ctx *ctx, int argc, char **argv) {
    (void)ctx;
    (void)argv;
    const char *usage = \
        "usage: get_font_smoothing\n";
    if (argc != 1) {
        fputs(usage, ctx->fout);
        return;
    }
    const char *names[] = {"off", "anti-aliasing", "subpixel"};
    COVERAGE_ON();
    int type = umka_sys_get_font_smoothing();
    COVERAGE_OFF();
    fprintf(ctx->fout, "font smoothing: %i - %s\n", type, names[type]);
}

static void
cmd_set_font_smoothing(struct shell_ctx *ctx, int argc, char **argv) {
    (void)ctx;
    const char *usage = \
        "usage: set_font_smoothing <mode>\n"
        "  mode             0 - off, 1 - gray AA, 2 - subpixel AA\n";
    if (argc != 2) {
        fputs(usage, ctx->fout);
        return;
    }
    int type = strtol(argv[1], NULL, 0);
    COVERAGE_ON();
    umka_sys_set_font_smoothing(type);
    COVERAGE_OFF();
}

static void
cmd_get_font_size(struct shell_ctx *ctx, int argc, char **argv) {
    (void)ctx;
    const char *usage = \
        "usage: get_font_size\n";
    if (argc != 1) {
        fputs(usage, ctx->fout);
        return;
    }
    (void)argv;
    COVERAGE_ON();
    size_t size = umka_sys_get_font_size();
    COVERAGE_OFF();
    fprintf(ctx->fout, "%upx\n", size);
}

static void
cmd_set_font_size(struct shell_ctx *ctx, int argc, char **argv) {
    (void)ctx;
    const char *usage = \
        "usage: set_font_size <size>\n"
        "  size             in pixels\n";
    if (argc != 2) {
        fputs(usage, ctx->fout);
        return;
    }
    uint32_t size = strtoul(argv[1], NULL, 0);
    COVERAGE_ON();
    umka_sys_set_font_size(size);
    COVERAGE_OFF();
}

static void
cmd_set_mouse_pos_screen(struct shell_ctx *ctx, int argc, char **argv) {
    (void)ctx;
    const char *usage = \
        "usage: set_mouse_pos_screen <x> <y>\n"
        "  x                in pixels\n"
        "  y                in pixels\n";
    if (argc != 3) {
        fputs(usage, ctx->fout);
        return;
    }
    struct point16s pos;
    pos.x = strtol(argv[1], NULL, 0);
    pos.y = strtol(argv[2], NULL, 0);
    COVERAGE_ON();
    umka_sys_set_mouse_pos_screen(pos);
    COVERAGE_OFF();
}

static void
cmd_get_mouse_pos_screen(struct shell_ctx *ctx, int argc, char **argv) {
    (void)ctx;
    (void)argv;
    const char *usage = \
        "usage: get_mouse_pos_screen\n";
    if (argc != 1) {
        fputs(usage, ctx->fout);
        return;
    }
    COVERAGE_ON();
    struct point16s pos = umka_sys_get_mouse_pos_screen();
    COVERAGE_OFF();
    fprintf(ctx->fout, "x y: %" PRIi16 " %" PRIi16 "\n", pos.x, pos.y);
}

static void
cmd_get_mouse_pos_window(struct shell_ctx *ctx, int argc, char **argv) {
    (void)ctx;
    (void)argv;
    const char *usage = \
        "usage: get_mouse_pos_window\n";
    if (argc != 1) {
        fputs(usage, ctx->fout);
        return;
    }
    COVERAGE_ON();
    struct point16s pos = umka_sys_get_mouse_pos_window();
    COVERAGE_OFF();
    if (pos.y < 0) {
        pos.x++;
    }
    fprintf(ctx->fout, "x y: %" PRIi16 " %" PRIi16 "\n", pos.x, pos.y);
}

static void
cmd_get_mouse_buttons_state(struct shell_ctx *ctx, int argc, char **argv) {
    (void)ctx;
    (void)argv;
    const char *usage = \
        "usage: get_mouse_buttons_state\n";
    if (argc != 1) {
        fputs(usage, ctx->fout);
        return;
    }
    COVERAGE_ON();
    struct mouse_state m = umka_sys_get_mouse_buttons_state();
    COVERAGE_OFF();
    fprintf(ctx->fout, "buttons held (left right middle 4th 5th): %u %u %u %u %u\n",
           m.bl_held, m.br_held, m.bm_held, m.b4_held, m.b5_held);
}

static void
cmd_get_mouse_buttons_state_events(struct shell_ctx *ctx, int argc, char **argv) {
    (void)ctx;
    (void)argv;
    const char *usage = \
        "usage: get_mouse_buttons_state_events\n";
    if (argc != 1) {
        fputs(usage, ctx->fout);
        return;
    }
    COVERAGE_ON();
    struct mouse_state_events m = umka_sys_get_mouse_buttons_state_events();
    COVERAGE_OFF();
    fprintf(ctx->fout, "buttons held (left right middle 4th 5th): %u %u %u %u %u\n",
           m.st.bl_held, m.st.br_held, m.st.bm_held, m.st.b4_held,
           m.st.b5_held);
    fprintf(ctx->fout, "buttons pressed (left right middle): %u %u %u\n",
           m.ev.bl_pressed, m.ev.br_pressed, m.ev.bm_pressed);
    fprintf(ctx->fout, "buttons released (left right middle): %u %u %u\n",
           m.ev.bl_released, m.ev.br_released, m.ev.bm_released);
    fprintf(ctx->fout, "left button double click: %u\n", m.ev.bl_dbclick);
    fprintf(ctx->fout, "scrolls used (vertical horizontal): %u %u\n",
           m.ev.vscroll_used, m.ev.hscroll_used);
}

static void
cmd_button(struct shell_ctx *ctx, int argc, char **argv) {
    const char *usage = \
        "usage: button <x> <xsize> <y> <ysize> <id> <color> <draw_button>"
            " <draw_frame>\n"
        "  x                x\n"
        "  xsize            may be size-1, check it\n"
        "  y                y\n"
        "  ysize            may be size-1, check it\n"
        "  id               24-bit\n"
        "  color            hex\n"
        "  draw_button      0/1\n"
        "  draw_frame       0/1\n";
    if (argc != 9) {
        fputs(usage, ctx->fout);
        return;
    }
    size_t x;
    shell_parse_uint(ctx, &x, argv[1]);
    size_t xsize = strtoul(argv[2], NULL, 0);
    size_t y     = strtoul(argv[3], NULL, 0);
    size_t ysize = strtoul(argv[4], NULL, 0);
    uint32_t button_id = strtoul(argv[5], NULL, 0);
    uint32_t color = strtoul(argv[6], NULL, 16);
    int draw_button = strtoul(argv[7], NULL, 0);
    int draw_frame = strtoul(argv[8], NULL, 0);
    COVERAGE_ON();
    umka_sys_button(x, xsize, y, ysize, button_id, draw_button, draw_frame,
                    color);
    COVERAGE_OFF();
}

static void
cmd_kos_sys_misc_init_heap(struct shell_ctx *ctx, int argc, char **argv) {
    (void)argv;
    const char *usage = \
        "usage: kos_sys_misc_init_heap\n";
    if (argc != 1) {
        fputs(usage, ctx->fout);
        return;
    }

    COVERAGE_ON();
    size_t heap_size = kos_sys_misc_init_heap();
    COVERAGE_OFF();
    fprintf(ctx->fout, "heap size = %u\n", heap_size);
}

static void
cmd_kos_sys_misc_load_file(struct shell_ctx *ctx, int argc, char **argv) {
    const char *usage = \
        "usage: kos_sys_misc_load_file <filename> [-h] [-p]\n"
        "  file             file in kolibri fs, e.g. /sys/pew/blah\n"
        "  -h               dump bytes in hex\n"
        "  -p               print pointers\n";
    if (argc < 2) {
        fputs(usage, ctx->fout);
        return;
    }
    const char *fname = argv[1];
    int show_hash = 0;
    int show_pointers = 0;
    optparse_init(&ctx->opts, argv+1);
    int opt;
    while ((opt = optparse(&ctx->opts, "hp")) != -1) {
        switch (opt) {
        case 'h':
            show_hash = 1;
            break;
        case 'p':
            show_pointers = 1;
            break;
        default:
            fputs(usage, ctx->fout);
            return;
        }
    }

    COVERAGE_ON();
    struct sys_load_file_ret ret = kos_sys_misc_load_file(fname);
    COVERAGE_OFF();
    if (show_pointers) {
        fprintf(ctx->fout, "file data = %p\n", ret.fdata);
    } else {
        fprintf(ctx->fout, "file data = %s\n", ret.fdata ? "non-zero" : "0");
    }
    if (show_hash) {
        print_hash(ctx, ret.fdata, ret.fsize);
    }
    fprintf(ctx->fout, "file size = %u\n", ret.fsize);
    shell_var_add_ptr(ctx, ret.fdata);
}

static void
cmd_load_cursor_from_file(struct shell_ctx *ctx, int argc, char **argv) {
    const char *usage = \
        "usage: load_cursor_from_file <file>\n"
        "  file             file in .cur format in kolibri fs\n";
    if (argc != 2) {
        fputs(usage, ctx->fout);
        return;
    }
    int show_pointers = 0;
    COVERAGE_ON();
    void *handle = umka_sys_load_cursor_from_file(argv[1]);
    COVERAGE_OFF();
    if (show_pointers) {
        fprintf(ctx->fout, "handle = %p\n", handle);
    } else {
        fprintf(ctx->fout, "handle = %s\n", handle ? "non-zero" : "0");
    }
    shell_var_add_ptr(ctx, handle);
}

static void
cmd_load_cursor_from_mem(struct shell_ctx *ctx, int argc, char **argv) {
    const char *usage = \
        "usage: load_cursor_from_mem <file>\n"
        "  file             file in .cur format in host fs\n";
    if (argc != 2) {
        fputs(usage, ctx->fout);
        return;
    }
    int show_pointers = 0;
    const char *fname = argv[1];
    void *fdata = host_load_file(fname);
    if (!fdata) {
        fprintf(ctx->fout, "[umka] Can't load file: %s\n", fname);
        return;
    }
    COVERAGE_ON();
    void *handle = umka_sys_load_cursor_from_mem(fdata);
    COVERAGE_OFF();
    free(fdata);
    if (show_pointers) {
        fprintf(ctx->fout, "handle = %p\n", handle);
    } else {
        fprintf(ctx->fout, "handle = %s\n", handle ? "non-zero" : "0");
    }
    shell_var_add_ptr(ctx, handle);
}

static void
cmd_set_cursor(struct shell_ctx *ctx, int argc, char **argv) {
    const char *usage = \
        "usage: set_cursor <handle>\n"
        "  handle           as returned by load_cursor* functions\n";
    if (argc != 2) {
        fputs(usage, ctx->fout);
        return;
    }
    int show_pointers = 0;
    void *handle;
    shell_parse_ptr(ctx, &handle, argv[1]);
    COVERAGE_ON();
    handle = umka_sys_set_cursor(handle);
    COVERAGE_OFF();
    if (show_pointers) {
        fprintf(ctx->fout, "prev handle = %p\n", handle);
    } else {
        fprintf(ctx->fout, "prev handle = %s\n", handle ? "non-zero" : "0");
    }
    shell_var_add_ptr(ctx, handle);
}

static void
cmd_put_image(struct shell_ctx *ctx, int argc, char **argv) {
    (void)ctx;
    const char *usage = \
        "usage: put_image <file> <xsize> <ysize> <x> <y>\n"
        "  file             file with rgb triplets\n"
        "  xsize            x size\n"
        "  ysize            y size\n"
        "  x                x coord\n"
        "  y                y coord\n";
    if (argc != 6) {
        fputs(usage, ctx->fout);
        return;
    }
    FILE *f = fopen(argv[1], "rb");
    fseek(f, 0, SEEK_END);
    size_t fsize = ftell(f);
    rewind(f);
    uint8_t *image = (uint8_t*)malloc(fsize);
    fread(image, fsize, 1, f);
    fclose(f);
    size_t xsize = strtoul(argv[2], NULL, 0);
    size_t ysize = strtoul(argv[3], NULL, 0);
    size_t x = strtoul(argv[4], NULL, 0);
    size_t y = strtoul(argv[5], NULL, 0);
    COVERAGE_ON();
    umka_sys_put_image(image, xsize, ysize, x, y);
    COVERAGE_OFF();
    free(image);
}

static void
cmd_put_image_palette(struct shell_ctx *ctx, int argc, char **argv) {
    (void)ctx;
    const char *usage = \
        "usage: put_image_palette <file> <xsize> <ysize> <x> <y> <bpp>"
            " <row_offset>\n"
        "  file             path/to/file, contents according tp bpp argument\n"
        "  xsize            x size\n"
        "  ysize            y size\n"
        "  x                x coord\n"
        "  y                y coord\n"
        "  bpp              bits per pixel\n"
        "  row_offset       in bytes\n";
    if (argc != 8) {
        fputs(usage, ctx->fout);
        return;
    }
    FILE *f = fopen(argv[1], "rb");
    fseek(f, 0, SEEK_END);
    size_t fsize = ftell(f);
    rewind(f);
    uint8_t *image = (uint8_t*)malloc(fsize);
    fread(image, fsize, 1, f);
    fclose(f);
    size_t xsize = strtoul(argv[2], NULL, 0);
    size_t ysize = strtoul(argv[3], NULL, 0);
    size_t x = strtoul(argv[4], NULL, 0);
    size_t y = strtoul(argv[5], NULL, 0);
    size_t bpp = strtoul(argv[6], NULL, 0);
    void *palette = NULL;
    size_t row_offset = strtoul(argv[7], NULL, 0);
    COVERAGE_ON();
    umka_sys_put_image_palette(image, xsize, ysize, x, y, bpp, palette,
                               row_offset);
    COVERAGE_OFF();
    free(image);
}

static void
cmd_draw_rect(struct shell_ctx *ctx, int argc, char **argv) {
    (void)ctx;
    const char *usage = \
        "usage: draw_rect <x> <xsize> <y> <ysize> <color> [-g]\n"
        "  x                x coord\n"
        "  xsize            x size\n"
        "  y                y coord\n"
        "  ysize            y size\n"
        "  color            in hex\n"
        "  -g               0/1 - gradient\n";
    if (argc < 6) {
        fputs(usage, ctx->fout);
        return;
    }
    size_t x     = strtoul(argv[1], NULL, 0);
    size_t xsize = strtoul(argv[2], NULL, 0);
    size_t y     = strtoul(argv[3], NULL, 0);
    size_t ysize = strtoul(argv[4], NULL, 0);
    uint32_t color = strtoul(argv[5], NULL, 16);
    int gradient = (argc == 7) && !strcmp(argv[6], "-g");
    COVERAGE_ON();
    umka_sys_draw_rect(x, xsize, y, ysize, color, gradient);
    COVERAGE_OFF();
}

static void
cmd_get_screen_size(struct shell_ctx *ctx, int argc, char **argv) {
    (void)ctx;
    (void)argv;
    const char *usage = \
        "usage: get_screen_size\n";
    if (argc != 1) {
        fputs(usage, ctx->fout);
        return;
    }
    uint32_t xsize, ysize;
    COVERAGE_ON();
    umka_sys_get_screen_size(&xsize, &ysize);
    COVERAGE_OFF();
    fprintf(ctx->fout, "%" PRIu32 "x%" PRIu32 "\n", xsize, ysize);
}

static void
cmd_draw_line(struct shell_ctx *ctx, int argc, char **argv) {
    (void)ctx;
    const char *usage = \
        "usage: draw_line <xbegin> <xend> <ybegin> <yend> <color> [-i]\n"
        "  xbegin           x left coord\n"
        "  xend             x right coord\n"
        "  ybegin           y top coord\n"
        "  yend             y bottom coord\n"
        "  color            hex\n"
        "  -i               inverted color\n";
    if (argc < 6) {
        fputs(usage, ctx->fout);
        return;
    }
    size_t x    = strtoul(argv[1], NULL, 0);
    size_t xend = strtoul(argv[2], NULL, 0);
    size_t y    = strtoul(argv[3], NULL, 0);
    size_t yend = strtoul(argv[4], NULL, 0);
    uint32_t color = strtoul(argv[5], NULL, 16);
    int invert = (argc == 7) && !strcmp(argv[6], "-i");
    COVERAGE_ON();
    umka_sys_draw_line(x, xend, y, yend, color, invert);
    COVERAGE_OFF();
}

static void
cmd_set_window_caption(struct shell_ctx *ctx, int argc, char **argv) {
    (void)ctx;
    const char *usage = \
        "usage: set_window_caption <caption> <encoding>\n"
        "  caption          asciiz string\n"
        "  encoding         1 = cp866, 2 = UTF-16LE, 3 = UTF-8\n";
    if (argc != 3) {
        fputs(usage, ctx->fout);
        return;
    }
    const char *caption = argv[1];
    int encoding = strtoul(argv[2], NULL, 0);
    COVERAGE_ON();
    umka_sys_set_window_caption(caption, encoding);
    COVERAGE_OFF();
}

static void
cmd_draw_window(struct shell_ctx *ctx, int argc, char **argv) {
    (void)ctx;
    const char *usage = \
        "usage: draw_window <x> <xsize> <y> <ysize> <color> <has_caption>"
            " <client_relative> <fill_workarea> <gradient_fill> <movable>"
            " <style> <caption>\n"
        "  x                x coord\n"
        "  xsize            x size\n"
        "  y                y coord\n"
        "  ysize            y size\n"
        "  color            hex\n"
        "  has_caption      0/1\n"
        "  client_relative  0/1\n"
        "  fill_workarea    0/1\n"
        "  gradient_fill    0/1\n"
        "  movable          0/1\n"
        "  style            1 - draw nothing, 3 - skinned, 4 - skinned fixed\n"
        "  caption          asciiz\n";
    if (argc != 13) {
        fputs(usage, ctx->fout);
        return;
    }
    size_t x     = strtoul(argv[1], NULL, 0);
    size_t xsize = strtoul(argv[2], NULL, 0);
    size_t y     = strtoul(argv[3], NULL, 0);
    size_t ysize = strtoul(argv[4], NULL, 0);
    uint32_t color = strtoul(argv[5], NULL, 16);
    int has_caption = strtoul(argv[6], NULL, 0);
    int client_relative = strtoul(argv[7], NULL, 0);
    int fill_workarea = strtoul(argv[8], NULL, 0);
    int gradient_fill = strtoul(argv[9], NULL, 0);
    int movable = strtoul(argv[10], NULL, 0);
    int style = strtoul(argv[11], NULL, 0);
    const char *caption = argv[12];
    COVERAGE_ON();
    umka_sys_draw_window(x, xsize, y, ysize, color, has_caption,
                         client_relative, fill_workarea, gradient_fill, movable,
                         style, caption);
    COVERAGE_OFF();
}

static void
cmd_window_redraw(struct shell_ctx *ctx, int argc, char **argv) {
    (void)ctx;
    const char *usage = \
        "usage: window_redraw <1|2>\n"
        "  1                begin\n"
        "  2                end\n";
    if (argc != 2) {
        fputs(usage, ctx->fout);
        return;
    }
    int begin_end = strtoul(argv[1], NULL, 0);
    COVERAGE_ON();
    umka_sys_window_redraw(begin_end);
    COVERAGE_OFF();
}

static void
cmd_move_window(struct shell_ctx *ctx, int argc, char **argv) {
    (void)ctx;
    const char *usage = \
        "usage: move_window <x> <y> <xsize> <ysize>\n"
        "  x                new x coord\n"
        "  y                new y coord\n"
        "  xsize            x size -1\n"
        "  ysize            y size -1\n";
    if (argc != 5) {
        fputs(usage, ctx->fout);
        return;
    }
    size_t x      = strtoul(argv[1], NULL, 0);
    size_t y      = strtoul(argv[2], NULL, 0);
    ssize_t xsize = strtol(argv[3], NULL, 0);
    ssize_t ysize = strtol(argv[4], NULL, 0);
    COVERAGE_ON();
    umka_sys_move_window(x, y, xsize, ysize);
    COVERAGE_OFF();
}

static void
cmd_blit_bitmap(struct shell_ctx *ctx, int argc, char **argv) {
    (void)ctx;
    const char *usage = \
        "usage: blit_bitmap <dstx> <dsty> <dstxsize> <dstysize> <srcx> <srcy>"
            " <srcxsize> <srcysize> <operation> <background> <transparent>"
            " <client_relative> <row_length>\n"
        "  dstx             dst rect x offset, window-relative\n"
        "  dsty             dst rect y offset, window-relative\n"
        "  dstxsize         dst rect width\n"
        "  dstysize         dst rect height\n"
        "  srcx             src rect x offset, window-relative\n"
        "  srcy             src rect y offset, window-relative\n"
        "  srcxsize         src rect width\n"
        "  srcysize         src rect height\n"
        "  operation        0 - copy\n"
        "  background       0/1 - blit into background surface\n"
        "  transparent      0/1\n"
        "  client_relative  0/1\n"
        "  row_length       in bytes\n";
    if (argc != 15) {
        fputs(usage, ctx->fout);
        return;
    }
    const char *fname = argv[1];
    FILE *f = fopen(fname, "rb");
    if (!f) {
        fprintf(ctx->fout, "[!] can't open file '%s': %s\n", fname, strerror(errno));
        return;
    }
    fseek(f, 0, SEEK_END);
    size_t fsize = ftell(f);
    rewind(f);
    uint8_t *image = (uint8_t*)malloc(fsize);
    fread(image, fsize, 1, f);
    fclose(f);
    size_t dstx     = strtoul(argv[2], NULL, 0);
    size_t dsty     = strtoul(argv[3], NULL, 0);
    size_t dstxsize = strtoul(argv[4], NULL, 0);
    size_t dstysize = strtoul(argv[5], NULL, 0);
    size_t srcx     = strtoul(argv[6], NULL, 0);
    size_t srcy     = strtoul(argv[7], NULL, 0);
    size_t srcxsize = strtoul(argv[8], NULL, 0);
    size_t srcysize = strtoul(argv[9], NULL, 0);
    int operation   = strtoul(argv[10], NULL, 0);
    int background  = strtoul(argv[11], NULL, 0);
    int transparent = strtoul(argv[12], NULL, 0);
    int client_relative = strtoul(argv[13], NULL, 0);
    int row_length = strtoul(argv[14], NULL, 0);
    uint32_t params[] = {dstx, dsty, dstxsize, dstysize, srcx, srcy, srcxsize,
                         srcysize, (uintptr_t)image, row_length};
    COVERAGE_ON();
    umka_sys_blit_bitmap(operation, background, transparent, client_relative,
                         params);
    COVERAGE_OFF();
    free(image);
}

static void
cmd_write_devices_dat(struct shell_ctx *ctx, int argc, char **argv) {
    (void)ctx;
    const char *usage = \
        "usage: write_devices_dat <file>\n"
        "  file             path/to/devices.dat\n";
    if (argc != 2) {
        fputs(usage, ctx->fout);
        return;
    }
    COVERAGE_ON();
    dump_devices_dat(argv[1]);
    COVERAGE_OFF();
}

static void
cmd_scrot(struct shell_ctx *ctx, int argc, char **argv) {
    (void)ctx;
    const char *usage = \
        "usage: scrot <file>\n"
        "  file             path/to/file in png format\n";
    if (argc != 2) {
        fputs(usage, ctx->fout);
        return;
    }

    uint32_t *lfb32 = (uint32_t*)kos_lfb_base;

    lfb32 = malloc(4*kos_display.width*kos_display.height);
    copy_display_to_rgb888(lfb32);

    uint8_t *from = (uint8_t*)lfb32;
    for (size_t y = 0; y < kos_display.height; y++) {
        for (size_t x = 0; x < kos_display.width; x++) {
            uint32_t p = 0;
            p += (uint32_t)from[y*kos_display.width*4+x*4 + 0] << 16;
            p += (uint32_t)from[y*kos_display.width*4+x*4 + 1] << 8;
            p += (uint32_t)from[y*kos_display.width*4+x*4 + 2] << 0;
            p += 0xff000000;
            ((uint32_t*)from)[y*kos_display.width+x] = p;
        }
    }

    unsigned error = lodepng_encode32_file(argv[1], (void*)lfb32,
                                           kos_display.width,
                                           kos_display.height);
    if (error) {
        fprintf(ctx->fout, "error %u: %s\n", error, lodepng_error_text(error));
    }
    free(lfb32);
}

static void
cmd_cd(struct shell_ctx *ctx, int argc, char **argv) {
    (void)ctx;
    const char *usage = \
        "usage: cd <path>\n"
        "  path             path/to/dir\n";
    if (argc != 2) {
        fputs(usage, ctx->fout);
        return;
    }
    COVERAGE_ON();
    umka_sys_set_cwd(argv[1]);
    COVERAGE_OFF();
    cur_dir_changed = true;
}

static void
ls_range(struct shell_ctx *ctx, f7080s1arg_t *fX0, f70or80_t f70or80) {
    f7080ret_t r;
    size_t bdfe_len = (fX0->encoding == CP866) ? BDFE_LEN_CP866 :
                                                 BDFE_LEN_UNICODE;
    uint32_t requested = fX0->size;
    if (fX0->size > MAX_DIRENTS_TO_READ) {
        fX0->size = MAX_DIRENTS_TO_READ;
    }
    for (; requested; requested -= fX0->size) {
        if (fX0->size > requested) {
            fX0->size = requested;
        }
        COVERAGE_ON();
        umka_sys_lfn(fX0, &r, f70or80);
        COVERAGE_OFF();
        fX0->offset += fX0->size;
        print_f70_status(ctx, &r, 1);
        f7080s1info_t *dir = fX0->buf;
        int ok = (r.count <= fX0->size);
        ok &= (dir->cnt == r.count);
        ok &= (r.status == KOS_ERROR_SUCCESS && r.count == fX0->size)
              || (r.status == KOS_ERROR_END_OF_FILE && r.count < fX0->size);
        assert(ok);
        if (!ok)
            break;
        bdfe_t *bdfe = dir->bdfes;
        for (size_t i = 0; i < dir->cnt; i++) {
            char fattr[KF_ATTR_CNT+1];
            convert_f70_file_attr(bdfe->attr, fattr);
            fprintf(ctx->fout, "%s %s\n", fattr, bdfe->name);
            bdfe = (bdfe_t*)((uintptr_t)bdfe + bdfe_len);
        }
        if (r.status == KOS_ERROR_END_OF_FILE) {
            break;
        }
    }
}

static void
ls_all(struct shell_ctx *ctx, f7080s1arg_t *fX0, f70or80_t f70or80) {
    f7080ret_t r;
    size_t bdfe_len = (fX0->encoding == CP866) ? BDFE_LEN_CP866 :
                                                 BDFE_LEN_UNICODE;
    while (true) {
        struct umka_cmd *cmd = umka_cmd_buf;
        struct cmd_sys_lfn_arg *c = &cmd->sys_lfn.arg;
        cmd->type = UMKA_CMD_SYS_LFN;
        c->f70or80 = f70or80;
        c->bufptr = fX0;
        c->r = &r;

        shell_run_cmd(ctx);
        atomic_store_explicit(&cmd->status, SHELL_CMD_STATUS_EMPTY,
                              memory_order_release);
        print_f70_status(ctx, &r, 1);
        assert((r.status == ERROR_SUCCESS && r.count == fX0->size)
              || (r.status == ERROR_END_OF_FILE && r.count < fX0->size));
        f7080s1info_t *dir = fX0->buf;
        fX0->offset += dir->cnt;
        int ok = (r.count <= fX0->size);
        ok &= (dir->cnt == r.count);
        ok &= (r.status == KOS_ERROR_SUCCESS && r.count == fX0->size)
              || (r.status == KOS_ERROR_END_OF_FILE && r.count < fX0->size);
        assert(ok);
        if (!ok)
            break;
        fprintf(ctx->fout, "total = %"PRIi32"\n", dir->total_cnt);
        bdfe_t *bdfe = dir->bdfes;
        for (size_t i = 0; i < dir->cnt; i++) {
            char fattr[KF_ATTR_CNT+1];
            convert_f70_file_attr(bdfe->attr, fattr);
            fprintf(ctx->fout, "%s %s\n", fattr, bdfe->name);
            bdfe = (bdfe_t*)((uintptr_t)bdfe + bdfe_len);
        }
        if (r.status == KOS_ERROR_END_OF_FILE) {
            break;
        }
    }
}

static fs_enc_t
parse_encoding(const char *str) {
    fs_enc_t enc;
    if (!strcmp(str, "default")) {
        enc = DEFAULT_ENCODING;
    } else if (!strcmp(str, "cp866")) {
        enc = CP866;
    } else if (!strcmp(str, "utf16")) {
        enc = UTF16;
    } else if (!strcmp(str, "utf8")) {
        enc = UTF8;
    } else {
        enc = INVALID_ENCODING;
    }
    return enc;
}

static void
cmd_exec(struct shell_ctx *ctx, int argc, char **argv) {
    (void)ctx;
    const char *usage = \
        "usage: exec <file>\n"
        "  file           executable to run\n";
    if (!argc) {
        fputs(usage, ctx->fout);
        return;
    }
    f7080s7arg_t fX0 = {.sf = 7};
    f7080ret_t r;
    int opt = 1;
    fX0.u.f70.zero = 0;
    fX0.u.f70.path = argv[opt++];
    fX0.flags = 0;
    fX0.params = "test";

    COVERAGE_ON();
    umka_sys_lfn(&fX0, &r, F70);
    COVERAGE_OFF();
    if (r.status < 0) {
        r.status = -r.status;
    } else {
        fprintf(ctx->fout, "pid: %" PRIu32 "\n", r.status);
        r.status = 0;
    }
    print_f70_status(ctx, &r, 1);
}

static void
cmd_ls(struct shell_ctx *ctx, int argc, char **argv, const char *usage,
       f70or80_t f70or80) {
    (void)ctx;
    if (!argc) {
        fputs(usage, ctx->fout);
        return;
    }
    int opt;
    const char *optstring = (f70or80 == F70) ? "f:c:e:" : "f:c:e:p:";
    const char *path = ".";
    uint32_t readdir_enc = DEFAULT_READDIR_ENCODING;
    uint32_t path_enc = DEFAULT_PATH_ENCODING;
    uint32_t from_idx = 0, count = MAX_DIRENTS_TO_READ;
    optparse_init(&ctx->opts, argv);
    if (argc > 1 && argv[1][0] != '-') {
        path = optparse_arg(&ctx->opts);
    }
    while ((opt = optparse(&ctx->opts, optstring)) != -1) {
        switch (opt) {
        case 'f':
            from_idx = strtoul(ctx->opts.optarg, NULL, 0);
            break;
        case 'c':
            count = strtoul(ctx->opts.optarg, NULL, 0);
            break;
        case 'e':
            readdir_enc = parse_encoding(ctx->opts.optarg);
            break;
        case 'p':
            path_enc = parse_encoding(ctx->opts.optarg);
            break;
        default:
            fputs(usage, ctx->fout);
            return;
        }
    }

    size_t bdfe_len = (readdir_enc <= CP866) ? BDFE_LEN_CP866 :
                                               BDFE_LEN_UNICODE;
    f7080s1info_t *dir = (f7080s1info_t*)malloc(sizeof(f7080s1info_t) +
                                                bdfe_len * MAX_DIRENTS_TO_READ);
    f7080s1arg_t fX0 = {.sf = 1, .offset = from_idx, .encoding = readdir_enc,
                        .size = count, .buf = dir};
    if (f70or80 == F70) {
        fX0.u.f70.zero = 0;
        fX0.u.f70.path = path;
    } else {
        fX0.u.f80.path_encoding = path_enc;
        fX0.u.f80.path = path;
    }
    if (count != MAX_DIRENTS_TO_READ) {
        ls_range(ctx, &fX0, f70or80);
    } else {
        ls_all(ctx, &fX0, f70or80);
    }
    free(dir);
    return;
}

static void
cmd_ls70(struct shell_ctx *ctx, int argc, char **argv) {
    const char *usage = \
        "usage: ls70 [dir] [option]...\n"
        "  -f number        index of the first dir entry to read\n"
        "  -c number        number of dir entries to read\n"
        "  -e encoding      cp866|utf16|utf8\n"
        "                   return directory listing in this encoding\n";
    cmd_ls(ctx, argc, argv, usage, F70);
}

static void
cmd_ls80(struct shell_ctx *ctx, int argc, char **argv) {
    const char *usage = \
        "usage: ls80 [dir] [option]...\n"
        "  -f number        index of the first dir entry to read\n"
        "  -c number        number of dir entries to read\n"
        "  -e encoding      cp866|utf16|utf8\n"
        "                   return directory listing in this encoding\n"
        "  -p encoding      cp866|utf16|utf8\n"
        "                   path to dir is specified in this encoding\n";
    cmd_ls(ctx, argc, argv, usage, F80);
}

static void
cmd_stat(struct shell_ctx *ctx, int argc, char **argv, f70or80_t f70or80) {
    const char *usage = \
        "usage: stat <file> [-c] [-m] [-a]\n"
        "  file             path/to/file\n"
        "  [-c]             force show creation time\n"
        "  [-m]             force show modification time\n"
        "  [-a]             force show access time\n";
    if (argc < 2) {
        fputs(usage, ctx->fout);
        return;
    }
    optparse_init(&ctx->opts, argv);
    bool force_ctime = false, force_mtime = false, force_atime = false;
    f7080s5arg_t fX0 = {.sf = 5, .flags = 0};
    f7080ret_t r;
    bdfe_t file;
    fX0.buf = &file;
    if (f70or80 == F70) {
        fX0.u.f70.zero = 0;
        fX0.u.f70.path = optparse_arg(&ctx->opts);
    } else {
        fX0.u.f80.path_encoding = DEFAULT_PATH_ENCODING;
        fX0.u.f80.path = optparse_arg(&ctx->opts);
    }
    COVERAGE_ON();
    umka_sys_lfn(&fX0, &r, f70or80);
    COVERAGE_OFF();
    print_f70_status(ctx, &r, 0);
    if (r.status != KOS_ERROR_SUCCESS)
        return;
    char fattr[KF_ATTR_CNT+1];
    convert_f70_file_attr(file.attr, fattr);
    fprintf(ctx->fout, "attr: %s\n", fattr);
    if ((file.attr & KF_FOLDER) == 0) {   // don't show size for dirs
        fprintf(ctx->fout, "size: %llu\n", file.size);
    }

    int opt;
    while ((opt = optparse(&ctx->opts, "cma")) != -1) {
        switch (opt) {
        case 'c':
            force_ctime = true;
            break;
        case 'm':
            force_mtime = true;
            break;
        case 'a':
            force_atime = true;
            break;
        default:
            fputs(usage, ctx->fout);
            return;
        }
    }

    time_t time;
    struct tm *t;
    if (!ctx->reproducible || force_atime) {
        time = kos_time_to_epoch(&file.atime);
        t = localtime(&time);
        fprintf(ctx->fout, "atime: %4.4i.%2.2i.%2.2i %2.2i:%2.2i:%2.2i\n",
               t->tm_year + 1900, t->tm_mon + 1, t->tm_mday,
               t->tm_hour, t->tm_min, t->tm_sec);
    }
    if (!ctx->reproducible || force_mtime) {
        time = kos_time_to_epoch(&file.mtime);
        t = localtime(&time);
        fprintf(ctx->fout, "mtime: %4.4i.%2.2i.%2.2i %2.2i:%2.2i:%2.2i\n",
               t->tm_year + 1900, t->tm_mon + 1, t->tm_mday,
               t->tm_hour, t->tm_min, t->tm_sec);
    }
    if (!ctx->reproducible || force_ctime) {
        time = kos_time_to_epoch(&file.ctime);
        t = localtime(&time);
        fprintf(ctx->fout, "ctime: %4.4i.%2.2i.%2.2i %2.2i:%2.2i:%2.2i\n",
               t->tm_year + 1900, t->tm_mon + 1, t->tm_mday,
               t->tm_hour, t->tm_min, t->tm_sec);
    }
    return;
}

static void
cmd_stat70(struct shell_ctx *ctx, int argc, char **argv) {
    cmd_stat(ctx, argc, argv, F70);
}

static void
cmd_stat80(struct shell_ctx *ctx, int argc, char **argv) {
    cmd_stat(ctx, argc, argv, F80);
}

static void
cmd_read(struct shell_ctx *ctx, int argc, char **argv, f70or80_t f70or80,
         const char *usage) {
    (void)ctx;
    if (argc < 3) {
        fputs(usage, ctx->fout);
        return;
    }
    f7080s0arg_t fX0 = {.sf = 0};
    f7080ret_t r;
    bool dump_bytes = false, dump_hash = false;
    int opt = 1;
    if (f70or80 == F70) {
        fX0.u.f70.zero = 0;
        fX0.u.f70.path = argv[opt++];
    } else {
        fX0.u.f80.path_encoding = DEFAULT_PATH_ENCODING;
        fX0.u.f80.path = argv[opt++];
    }
    if ((opt >= argc) || !parse_uint64(ctx, argv[opt++], &fX0.offset))
        return;
    if ((opt >= argc) || !parse_uint32(ctx, argv[opt++], &fX0.count))
        return;
    for (; opt < argc; opt++) {
        if (!strcmp(argv[opt], "-b")) {
            dump_bytes = true;
        } else if (!strcmp(argv[opt], "-h")) {
            dump_hash = true;
        } else if (!strcmp(argv[opt], "-e")) {
            if (f70or80 == F70) {
                fprintf(ctx->fout, "f70 doesn't accept encoding parameter,"
                        " use f80\n");
                return;
            }
        } else {
            fprintf(ctx->fout, "invalid option: '%s'\n", argv[opt]);
            return;
        }
    }
    fX0.buf = (uint8_t*)malloc(fX0.count);

    COVERAGE_ON();
    umka_sys_lfn(&fX0, &r, f70or80);
    COVERAGE_OFF();

    print_f70_status(ctx, &r, 1);
    if (r.status == KOS_ERROR_SUCCESS || r.status == KOS_ERROR_END_OF_FILE) {
        if (dump_bytes)
            print_bytes(ctx, fX0.buf, r.count);
        if (dump_hash)
            print_hash(ctx, fX0.buf, r.count);
    }

    free(fX0.buf);
    return;
}

static void
cmd_read70(struct shell_ctx *ctx, int argc, char **argv) {
    const char *usage = \
        "usage: read70 <file> <offset> <length> [-b] [-h]\n"
        "  file             path/to/file\n"
        "  offset           in bytes\n"
        "  length           in bytes\n"
        "  -b               dump bytes in hex\n"
        "  -h               print hash of data read\n";

    cmd_read(ctx, argc, argv, F70, usage);
}

static void
cmd_read80(struct shell_ctx *ctx, int argc, char **argv) {
    const char *usage = \
        "usage: read80 <file> <offset> <length> [-b] [-h]"
            " [-e cp866|utf8|utf16]\n"
        "  file             path/to/file\n"
        "  offset           in bytes\n"
        "  length           in bytes\n"
        "  -b               dump bytes in hex\n"
        "  -h               print hash of data read\n"
        "  -e               encoding\n";
    cmd_read(ctx, argc, argv, F80, usage);
}

static void
cmd_acpi_preload_table(struct shell_ctx *ctx, int argc, char **argv) {
    (void)ctx;
    const char *usage = \
        "usage: acpi_preload_table <file>\n"
        "  file             path/to/local/file.aml\n";
    if (argc != 2) {
        fputs(usage, ctx->fout);
        return;
    }
    FILE *f = fopen(argv[1], "rb");
    if (!f) {
        fprintf(ctx->fout, "[umka] can't open file: %s\n", argv[1]);
        return;
    }
    fseek(f, 0, SEEK_END);
    size_t fsize = ftell(f);
    rewind(f);
    uint8_t *table = (uint8_t*)malloc(fsize);
    fread(table, fsize, 1, f);
    fclose(f);
    fprintf(ctx->fout, "table #%zu\n", kos_acpi_ssdt_cnt);
    kos_acpi_ssdt_base[kos_acpi_ssdt_cnt] = table;
    kos_acpi_ssdt_size[kos_acpi_ssdt_cnt] = fsize;
    kos_acpi_ssdt_cnt++;
}

static void
cmd_acpi_enable(struct shell_ctx *ctx, int argc, char **argv) {
    (void)ctx;
    const char *usage = \
        "usage: acpi_enable\n";
    if (argc != 1) {
        fputs(usage, ctx->fout);
        return;
    }
    (void)argv;
    COVERAGE_ON();
    kos_enable_acpi();
    COVERAGE_OFF();
}

static void
cmd_acpi_get_node_cnt(struct shell_ctx *ctx, int argc, char **argv) {
    (void)ctx;
    (void)argv;
    const char *usage = \
        "usage: acpi_get_node_cnt\n";
    if (argc != 1) {
        fputs(usage, ctx->fout);
        return;
    }
    uint32_t cnt = kos_acpi_count_nodes(acpi_ctx);
    fprintf(ctx->fout, "nodes in namespace: %" PRIu32 "\n", cnt);
}

static void
cmd_acpi_get_node_alloc_cnt(struct shell_ctx *ctx, int argc, char **argv) {
    (void)ctx;
    (void)argv;
    const char *usage = \
        "usage: acpi_get_node_alloc_cnt\n";
    if (argc != 1) {
        fputs(usage, ctx->fout);
        return;
    }
    fprintf(ctx->fout, "nodes allocated: %" PRIu32 "\n", kos_acpi_node_alloc_cnt);
}

static void
cmd_acpi_get_node_free_cnt(struct shell_ctx *ctx, int argc, char **argv) {
    (void)ctx;
    (void)argv;
    const char *usage = \
        "usage: acpi_get_node_free_cnt\n";
    if (argc != 1) {
        fputs(usage, ctx->fout);
        return;
    }
    fprintf(ctx->fout, "nodes freed: %" PRIu32 "\n", kos_acpi_node_free_cnt);
}

static void
cmd_acpi_set_usage(struct shell_ctx *ctx, int argc, char **argv) {
    (void)ctx;
    const char *usage = \
        "usage: acpi_set_usage <num>\n"
        "  num            one of ACPI_USAGE_* constants\n";
    if (argc != 2) {
        fputs(usage, ctx->fout);
        return;
    }
    uint32_t acpi_usage = strtoul(argv[1], NULL, 0);
    kos_acpi_usage = acpi_usage;
}

static void
cmd_acpi_get_usage(struct shell_ctx *ctx, int argc, char **argv) {
    (void)ctx;
    (void)argv;
    const char *usage = \
        "usage: acpi_get_usage\n";
    if (argc != 1) {
        fputs(usage, ctx->fout);
        return;
    }
    fprintf(ctx->fout, "ACPI usage: %" PRIu32 "\n", kos_acpi_usage);
}

static void
cmd_acpi_call(struct shell_ctx *ctx, int argc, char **argv) {
    (void)ctx;
    const char *usage = \
        "usage: acpi_call <method> [args...]\n"
        "  method         name of acpi method to call, e.g. \\_SB_PCI0_PRT\n";
    if (argc > 2) {
        puts("arguments are not supported / not implemented!");
        fputs(usage, ctx->fout);
        return;
    }
    if (argc < 2) {
        fputs(usage, ctx->fout);
        return;
    }
    const char *method = argv[1];
    fprintf(ctx->fout, "calling acpi method: '%s'\n", method);
    COVERAGE_ON();
    kos_acpi_call_name(acpi_ctx, method);
    COVERAGE_OFF();
    fprintf(ctx->fout, "acpi method returned\n");
}

static void
cmd_pci_set_path(struct shell_ctx *ctx, int argc, char **argv) {
    (void)ctx;
    const char *usage = \
        "usage: pci_set_path <path>\n"
        "  path           where aaaa:bb:cc.d dirs are\n";
    if (argc != 2) {
        fputs(usage, ctx->fout);
        return;
    }
    strcpy(pci_path, argv[1]);
}

static void
cmd_pci_get_path(struct shell_ctx *ctx, int argc, char **argv) {
    (void)ctx;
    (void)argv;
    const char *usage = \
        "usage: pci_get_path\n";
    if (argc != 1) {
        fputs(usage, ctx->fout);
        return;
    }
    fprintf(ctx->fout, "pci path: %s\n", pci_path);
}

static void
cmd_load_dll(struct shell_ctx *ctx, int argc, char **argv) {
    (void)ctx;
    const char *usage = \
        "usage: load_dll <path>\n"
        "  path           /path/to/library.obj in KolibriOS fs\n";
    if (argc != 2) {
        fputs(usage, ctx->fout);
        return;
    }
    COVERAGE_ON();
    void *export = umka_sys_load_dll(argv[1]);
    COVERAGE_OFF();
//    if (ctx->reproducible)
    fprintf(ctx->fout, "### export: %p\n", export);
}

static void
cmd_stack_init(struct shell_ctx *ctx, int argc, char **argv) {
    (void)ctx;
    (void)argv;
    const char *usage = \
        "usage: stack_init\n";
    if (argc != 1) {
        fputs(usage, ctx->fout);
        return;
    }
    COVERAGE_ON();
    umka_stack_init();
    COVERAGE_OFF();
}

static void
cmd_net_add_device(struct shell_ctx *ctx, int argc, char **argv) {
    (void)ctx;
    (void)argv;
    const char *usage = \
        "usage: net_add_device <devtype>\n"
        "  devtype          null, file or tap\n";
    if (argc > 2) {
        fputs(usage, ctx->fout);
        return;
    }
    int devtype = VNET_DEVTYPE_NULL;
    const char *devtypestr = argv[1];
    if (devtypestr) {
        if (!strcmp(devtypestr, "null")) {
            devtype = VNET_DEVTYPE_NULL;
        } else if (!strcmp(devtypestr, "file")) {
            devtype = VNET_DEVTYPE_FILE;
        } else if (!strcmp(devtypestr, "tap")) {
            devtype = VNET_DEVTYPE_TAP;
        } else {
            fprintf(ctx->fout, "bad device type: %s\n", devtypestr);
            return;
        }
    }
    struct vnet *vnet = vnet_init(devtype, ctx->running); // TODO: list like block devices
    COVERAGE_ON();
    int32_t dev_num = kos_net_add_device(&vnet->eth.net);
    COVERAGE_OFF();
    fprintf(ctx->fout, "device number: %" PRIi32 "\n", dev_num);
}

static void
cmd_net_get_dev_count(struct shell_ctx *ctx, int argc, char **argv) {
    (void)ctx;
    const char *usage = \
        "usage: net_get_dev_count\n";
    if (argc != 1) {
        fputs(usage, ctx->fout);
        return;
    }
    (void)argv;
    COVERAGE_ON();
    uint32_t count = umka_sys_net_get_dev_count();
    COVERAGE_OFF();
    fprintf(ctx->fout, "active network devices: %u\n", count);
}

static void
cmd_net_get_dev_type(struct shell_ctx *ctx, int argc, char **argv) {
    (void)ctx;
    const char *usage = \
        "usage: net_get_dev_type <dev_num>\n"
        "  dev_num        device number as returned by net_add_device\n";
    if (argc != 2) {
        fputs(usage, ctx->fout);
        return;
    }
    uint8_t dev_num = strtoul(argv[1], NULL, 0);
    COVERAGE_ON();
    int32_t dev_type = umka_sys_net_get_dev_type(dev_num);
    COVERAGE_OFF();
    fprintf(ctx->fout, "status: %s\n", dev_type == -1 ? "fail" : "ok");
    if (dev_type != -1) {
        fprintf(ctx->fout, "type of network device #%" PRIu8 ": %i\n",
               dev_num, dev_type);
    }
}

static void
cmd_net_get_dev_name(struct shell_ctx *ctx, int argc, char **argv) {
    (void)ctx;
    const char *usage = \
        "usage: net_get_dev_name <dev_num>\n"
        "  dev_num        device number as returned by net_add_device\n";
    if (argc != 2) {
        fputs(usage, ctx->fout);
        return;
    }
    char dev_name[64];
    uint8_t dev_num = strtoul(argv[1], NULL, 0);
    COVERAGE_ON();
    int32_t status = umka_sys_net_get_dev_name(dev_num, dev_name);
    COVERAGE_OFF();
    fprintf(ctx->fout, "status: %s\n", status == -1 ? "fail" : "ok");
    if (status != -1) {
        fprintf(ctx->fout, "name of network device #%" PRIu8 ": %s\n",
               dev_num, dev_name);
    }
}

static void
cmd_net_dev_reset(struct shell_ctx *ctx, int argc, char **argv) {
    (void)ctx;
    const char *usage = \
        "usage: net_dev_reset <dev_num>\n"
        "  dev_num        device number as returned by net_add_device\n";
    if (argc != 2) {
        fputs(usage, ctx->fout);
        return;
    }
    uint8_t dev_num = strtoul(argv[1], NULL, 0);
    COVERAGE_ON();
    int32_t status = umka_sys_net_dev_reset(dev_num);
    COVERAGE_OFF();
    fprintf(ctx->fout, "status: %s\n", status == -1 ? "fail" : "ok");
}

static void
cmd_net_dev_stop(struct shell_ctx *ctx, int argc, char **argv) {
    (void)ctx;
    const char *usage = \
        "usage: net_dev_stop <dev_num>\n"
        "  dev_num        device number as returned by net_add_device\n";
    if (argc != 2) {
        fputs(usage, ctx->fout);
        return;
    }
    uint8_t dev_num = strtoul(argv[1], NULL, 0);
    COVERAGE_ON();
    int32_t status = umka_sys_net_dev_stop(dev_num);
    COVERAGE_OFF();
    fprintf(ctx->fout, "status: %s\n", status == -1 ? "fail" : "ok");
}

static void
cmd_net_get_dev(struct shell_ctx *ctx, int argc, char **argv) {
    (void)ctx;
    const char *usage = \
        "usage: net_get_dev <dev_num>\n"
        "  dev_num        device number as returned by net_add_device\n";
    if (argc != 2) {
        fputs(usage, ctx->fout);
        return;
    }
    uint8_t dev_num = strtoul(argv[1], NULL, 0);
    COVERAGE_ON();
    intptr_t dev = umka_sys_net_get_dev(dev_num);
    COVERAGE_OFF();
    fprintf(ctx->fout, "status: %s\n", dev == -1 ? "fail" : "ok");
    if (!ctx->reproducible && dev != -1) {
        fprintf(ctx->fout, "address of net dev #%" PRIu8 ": 0x%x\n", dev_num, dev);
    }
}

static void
cmd_net_get_packet_tx_count(struct shell_ctx *ctx, int argc, char **argv) {
    (void)ctx;
    const char *usage = \
        "usage: net_get_packet_tx_count <dev_num>\n"
        "  dev_num        device number as returned by net_add_device\n";
    if (argc != 2) {
        fputs(usage, ctx->fout);
        return;
    }
    uint8_t dev_num = strtoul(argv[1], NULL, 0);
    COVERAGE_ON();
    uint32_t count = umka_sys_net_get_packet_tx_count(dev_num);
    COVERAGE_OFF();
    fprintf(ctx->fout, "status: %s\n", count == UINT32_MAX ? "fail" : "ok");
    if (count != UINT32_MAX) {
        fprintf(ctx->fout, "packet tx count of net dev #%" PRIu8 ": %" PRIu32 "\n",
               dev_num, count);
    }
}

static void
cmd_net_get_packet_rx_count(struct shell_ctx *ctx, int argc, char **argv) {
    (void)ctx;
    const char *usage = \
        "usage: net_get_packet_rx_count <dev_num>\n"
        "  dev_num        device number as returned by net_add_device\n";
    if (argc != 2) {
        fputs(usage, ctx->fout);
        return;
    }
    uint8_t dev_num = strtoul(argv[1], NULL, 0);
    COVERAGE_ON();
    uint32_t count = umka_sys_net_get_packet_rx_count(dev_num);
    COVERAGE_OFF();
    fprintf(ctx->fout, "status: %s\n", count == UINT32_MAX ? "fail" : "ok");
    if (count != UINT32_MAX) {
        fprintf(ctx->fout, "packet rx count of net dev #%" PRIu8 ": %" PRIu32 "\n",
               dev_num, count);
    }
}

static void
cmd_net_get_byte_tx_count(struct shell_ctx *ctx, int argc, char **argv) {
    (void)ctx;
    const char *usage = \
        "usage: net_get_byte_tx_count <dev_num>\n"
        "  dev_num        device number as returned by net_add_device\n";
    if (argc != 2) {
        fputs(usage, ctx->fout);
        return;
    }
    uint8_t dev_num = strtoul(argv[1], NULL, 0);
    COVERAGE_ON();
    uint32_t count = umka_sys_net_get_byte_tx_count(dev_num);
    COVERAGE_OFF();
    fprintf(ctx->fout, "status: %s\n", count == UINT32_MAX ? "fail" : "ok");
    if (count != UINT32_MAX) {
        fprintf(ctx->fout, "byte tx count of net dev #%" PRIu8 ": %" PRIu32 "\n",
               dev_num, count);
    }
}

static void
cmd_net_get_byte_rx_count(struct shell_ctx *ctx, int argc, char **argv) {
    (void)ctx;
    const char *usage = \
        "usage: net_get_byte_rx_count <dev_num>\n"
        "  dev_num        device number as returned by net_add_device\n";
    if (argc != 2) {
        fputs(usage, ctx->fout);
        return;
    }
    uint8_t dev_num = strtoul(argv[1], NULL, 0);
    COVERAGE_ON();
    uint32_t count = umka_sys_net_get_byte_rx_count(dev_num);
    COVERAGE_OFF();
    fprintf(ctx->fout, "status: %s\n", count == UINT32_MAX ? "fail" : "ok");
    if (count != UINT32_MAX) {
        fprintf(ctx->fout, "byte rx count of net dev #%" PRIu8 ": %" PRIu32 "\n",
               dev_num, count);
    }
}

static void
print_link_status_names(struct shell_ctx *ctx, uint32_t status) {
    switch (status & 0x3) {
    case ETH_LINK_DOWN:
        fprintf(ctx->fout, "ETH_LINK_DOWN");
        break;
    case ETH_LINK_UNKNOWN:
        fprintf(ctx->fout, "ETH_LINK_UNKNOWN");
        break;
    case ETH_LINK_FD:
        fprintf(ctx->fout, "ETH_LINK_FD");
        break;
    default:
        fprintf(ctx->fout, "ERROR");
        break;
    }

    switch(status & ~3u) {
    case ETH_LINK_1G:
        fprintf(ctx->fout, " + ETH_LINK_1G");
        break;
    case ETH_LINK_100M:
        fprintf(ctx->fout, " + ETH_LINK_100M");
        break;
    case ETH_LINK_10M:
        fprintf(ctx->fout, " + ETH_LINK_10M");
        break;
    default:
        fprintf(ctx->fout, " + UNKNOWN");
        break;
    }
}

static void
cmd_net_get_link_status(struct shell_ctx *ctx, int argc, char **argv) {
    (void)ctx;
    const char *usage = \
        "usage: net_get_link_status <dev_num>\n"
        "  dev_num        device number as returned by net_add_device\n";
    if (argc != 2) {
        fputs(usage, ctx->fout);
        return;
    }
    uint8_t dev_num = strtoul(argv[1], NULL, 0);
    COVERAGE_ON();
    uint32_t status = umka_sys_net_get_link_status(dev_num);
    COVERAGE_OFF();
    fprintf(ctx->fout, "status: %s\n", status == UINT32_MAX ? "fail" : "ok");
    if (status != UINT32_MAX) {
        fprintf(ctx->fout, "link status of net dev #%" PRIu8 ": %" PRIu32 " ",
                dev_num, status);
        print_link_status_names(ctx, status);
        fprintf(ctx->fout, "\n");
    }
}

static void
cmd_net_open_socket(struct shell_ctx *ctx, int argc, char **argv) {
    (void)ctx;
    const char *usage = \
        "usage: net_open_socket <domain> <type> <protocol>\n"
        "  domain         domain\n"
        "  type           type\n"
        "  protocol       protocol\n";
    if (argc != 4) {
        fputs(usage, ctx->fout);
        return;
    }
    uint32_t domain   = strtoul(argv[1], NULL, 0);
    uint32_t type     = strtoul(argv[2], NULL, 0);
    uint32_t protocol = strtoul(argv[3], NULL, 0);
    COVERAGE_ON();
    f75ret_t r = umka_sys_net_open_socket(domain, type, protocol);
    COVERAGE_OFF();
    fprintf(ctx->fout, "value: 0x%" PRIx32 "\n", r.value);
    fprintf(ctx->fout, "errorcode: 0x%" PRIx32 "\n", r.errorcode);
// UINT32_MAX
}

static void
cmd_net_close_socket(struct shell_ctx *ctx, int argc, char **argv) {
    (void)ctx;
    const char *usage = \
        "usage: net_close_socket <socknum>\n"
        "  socknum        socket number\n";
    if (argc != 2) {
        fputs(usage, ctx->fout);
        return;
    }
    uint32_t fd = strtoul(argv[1], NULL, 0);
    COVERAGE_ON();
    f75ret_t r = umka_sys_net_close_socket(fd);
    COVERAGE_OFF();
    fprintf(ctx->fout, "value: 0x%" PRIx32 "\n", r.value);
    fprintf(ctx->fout, "errorcode: 0x%" PRIx32 "\n", r.errorcode);
}

static void
cmd_net_bind(struct shell_ctx *ctx, int argc, char **argv) {
    (void)ctx;
    const char *usage = \
        "usage: net_bind <fd> <port> <ip>\n"
        "  fd             socket number\n"
        "  port           port\n"
        "  addr           addr\n";
    if (argc != 4) {
        fputs(usage, ctx->fout);
        return;
    }
    uint32_t fd = strtoul(argv[1], NULL, 0);
    uint16_t port = strtoul(argv[2], NULL, 0);
    char *addr_str = argv[3];
    uint32_t addr = inet_addr(addr_str);
    struct sockaddr_in sa;
    memset(&sa, 0, sizeof(sa));
    sa.sin_family = AF_INET4;
    sa.sin_port = htons(port);
    sa.sin_addr.s_addr = addr;
    fprintf(ctx->fout, "sockaddr at %p\n", (void*)&sa);
    COVERAGE_ON();
    f75ret_t r = umka_sys_net_bind(fd, &sa, sizeof(struct sockaddr_in));
    COVERAGE_OFF();
    fprintf(ctx->fout, "value: 0x%" PRIx32 "\n", r.value);
    fprintf(ctx->fout, "errorcode: 0x%" PRIx32 "\n", r.errorcode);
}

static void
cmd_net_listen(struct shell_ctx *ctx, int argc, char **argv) {
    (void)ctx;
    const char *usage = \
        "usage: net_listen <fd> <backlog>\n"
        "  fd             socket number\n"
        "  backlog        max queue length\n";
    if (argc != 3) {
        fputs(usage, ctx->fout);
        return;
    }
    uint32_t fd = strtoul(argv[1], NULL, 0);
    uint32_t backlog = strtoul(argv[2], NULL, 0);
    COVERAGE_ON();
    f75ret_t r = umka_sys_net_listen(fd, backlog);
    COVERAGE_OFF();
    fprintf(ctx->fout, "value: 0x%" PRIx32 "\n", r.value);
    fprintf(ctx->fout, "errorcode: 0x%" PRIx32 "\n", r.errorcode);
}

static void
cmd_net_connect(struct shell_ctx *ctx, int argc, char **argv) {
    (void)ctx;
    const char *usage = \
        "usage: net_connect <fd> <port> <ip>\n"
        "  fd             socket number\n"
        "  port           port\n"
        "  addr           addr\n";
    if (argc != 4) {
        fputs(usage, ctx->fout);
        return;
    }
    uint32_t fd = strtoul(argv[1], NULL, 0);
    uint16_t port = strtoul(argv[2], NULL, 0);
    char *addr_str = argv[3];
    uint32_t addr = inet_addr(addr_str);
    struct sockaddr_in sa;
    memset(&sa, 0, sizeof(sa));
    sa.sin_family = AF_INET4;
    sa.sin_port = htons(port);
    sa.sin_addr.s_addr = addr;
    fprintf(ctx->fout, "sockaddr at %p\n", (void*)&sa);
    COVERAGE_ON();
    f75ret_t r = umka_sys_net_connect(fd, &sa, sizeof(struct sockaddr_in));
    COVERAGE_OFF();
    fprintf(ctx->fout, "value: 0x%" PRIx32 "\n", r.value);
    fprintf(ctx->fout, "errorcode: 0x%" PRIx32 "\n", r.errorcode);
}

static void
cmd_net_accept(struct shell_ctx *ctx, int argc, char **argv) {
    (void)ctx;
    const char *usage = \
        "usage: net_accept <fd> <port> <ip>\n"
        "  fd             socket number\n"
        "  port           port\n"
        "  addr           addr\n";
    if (argc != 4) {
        fputs(usage, ctx->fout);
        return;
    }
    uint32_t fd = strtoul(argv[1], NULL, 0);
    uint16_t port = strtoul(argv[2], NULL, 0);
    char *addr_str = argv[3];
    uint32_t addr = inet_addr(addr_str);
    struct sockaddr_in sa;
    memset(&sa, 0, sizeof(sa));
    sa.sin_family = AF_INET4;
    sa.sin_port = htons(port);
    sa.sin_addr.s_addr = addr;
    fprintf(ctx->fout, "sockaddr at %p\n", (void*)&sa);
    COVERAGE_ON();
    f75ret_t r = umka_sys_net_accept(fd, &sa, sizeof(struct sockaddr_in));
    COVERAGE_OFF();
    fprintf(ctx->fout, "value: 0x%" PRIx32 "\n", r.value);
    fprintf(ctx->fout, "errorcode: 0x%" PRIx32 "\n", r.errorcode);
}

static void
cmd_net_eth_read_mac(struct shell_ctx *ctx, int argc, char **argv) {
    (void)ctx;
    const char *usage = \
        "usage: net_eth_read_mac <dev_num>\n"
        "  dev_num        device number as returned by net_add_device\n";
    if (argc != 2) {
        fputs(usage, ctx->fout);
        return;
    }
    uint32_t dev_num = strtoul(argv[1], NULL, 0);
    COVERAGE_ON();
    f76ret_t r = umka_sys_net_eth_read_mac(dev_num);
    COVERAGE_OFF();
    if (r.eax == UINT32_MAX) {
        fprintf(ctx->fout, "status: fail\n");
    } else {
        fprintf(ctx->fout, "%2.2x:%2.2x:%2.2x:%2.2x:%2.2x:%2.2x\n",
               (uint8_t)(r.ebx >>  0), (uint8_t)(r.ebx >>  8),
               (uint8_t)(r.eax >>  0), (uint8_t)(r.eax >>  8),
               (uint8_t)(r.eax >> 16), (uint8_t)(r.eax >> 24));
    }
}

static void
cmd_net_ipv4_get_addr(struct shell_ctx *ctx, int argc, char **argv) {
    (void)ctx;
    const char *usage = \
        "usage: net_ipv4_get_addr <dev_num>\n"
        "  dev_num        device number as returned by net_add_device\n";
    if (argc != 2) {
        fputs(usage, ctx->fout);
        return;
    }
    uint32_t dev_num = strtoul(argv[1], NULL, 0);
    COVERAGE_ON();
    f76ret_t r = umka_sys_net_ipv4_get_addr(dev_num);
    COVERAGE_OFF();
    if (r.eax == UINT32_MAX) {
        fprintf(ctx->fout, "status: fail\n");
    } else {
        fprintf(ctx->fout, "%d.%d.%d.%d\n",
               (uint8_t)(r.eax >>  0), (uint8_t)(r.eax >>  8),
               (uint8_t)(r.eax >> 16), (uint8_t)(r.eax >> 24));
    }
}

static void
cmd_net_ipv4_set_addr(struct shell_ctx *ctx, int argc, char **argv) {
    (void)ctx;
    const char *usage = \
        "usage: net_ipv4_set_addr <dev_num> <addr>\n"
        "  dev_num        device number as returned by net_add_device\n"
        "  addr           a.b.c.d\n";
    if (argc != 3) {
        fputs(usage, ctx->fout);
        return;
    }
    uint32_t dev_num = strtoul(argv[1], NULL, 0);
    char *addr_str = argv[2];
    uint32_t addr = inet_addr(addr_str);
    COVERAGE_ON();
    f76ret_t r = umka_sys_net_ipv4_set_addr(dev_num, addr);
    COVERAGE_OFF();
    if (r.eax == UINT32_MAX) {
        fprintf(ctx->fout, "status: fail\n");
    } else {
        fprintf(ctx->fout, "status: ok\n");
    }
}

static void
cmd_net_ipv4_get_dns(struct shell_ctx *ctx, int argc, char **argv) {
    (void)ctx;
    const char *usage = \
        "usage: net_ipv4_get_dns <dev_num>\n"
        "  dev_num        device number as returned by net_add_device\n";
    if (argc != 2) {
        fputs(usage, ctx->fout);
        return;
    }
    uint32_t dev_num = strtoul(argv[1], NULL, 0);
    COVERAGE_ON();
    f76ret_t r = umka_sys_net_ipv4_get_dns(dev_num);
    COVERAGE_OFF();
    if (r.eax == UINT32_MAX) {
        fprintf(ctx->fout, "status: fail\n");
    } else {
        fprintf(ctx->fout, "%d.%d.%d.%d\n",
               (uint8_t)(r.eax >>  0), (uint8_t)(r.eax >>  8),
               (uint8_t)(r.eax >> 16), (uint8_t)(r.eax >> 24));
    }
}

static void
cmd_net_ipv4_set_dns(struct shell_ctx *ctx, int argc, char **argv) {
    (void)ctx;
    const char *usage = \
        "usage: net_ipv4_set_dns <dev_num> <dns>\n"
        "  dev_num        device number as returned by net_add_device\n"
        "  dns            a.b.c.d\n";
    if (argc != 3) {
        fputs(usage, ctx->fout);
        return;
    }
    uint32_t dev_num = strtoul(argv[1], NULL, 0);
    uint32_t dns = inet_addr(argv[2]);
    COVERAGE_ON();
    f76ret_t r = umka_sys_net_ipv4_set_dns(dev_num, dns);
    COVERAGE_OFF();
    if (r.eax == UINT32_MAX) {
        fprintf(ctx->fout, "status: fail\n");
    } else {
        fprintf(ctx->fout, "status: ok\n");
    }
}

static void
cmd_net_ipv4_get_subnet(struct shell_ctx *ctx, int argc, char **argv) {
    (void)ctx;
    const char *usage = \
        "usage: net_ipv4_get_subnet <dev_num>\n"
        "  dev_num        device number as returned by net_add_device\n";
    if (argc != 2) {
        fputs(usage, ctx->fout);
        return;
    }
    uint32_t dev_num = strtoul(argv[1], NULL, 0);
    COVERAGE_ON();
    f76ret_t r = umka_sys_net_ipv4_get_subnet(dev_num);
    COVERAGE_OFF();
    if (r.eax == UINT32_MAX) {
        fprintf(ctx->fout, "status: fail\n");
    } else {
        fprintf(ctx->fout, "%d.%d.%d.%d\n",
               (uint8_t)(r.eax >>  0), (uint8_t)(r.eax >>  8),
               (uint8_t)(r.eax >> 16), (uint8_t)(r.eax >> 24));
    }
}

static void
cmd_net_ipv4_set_subnet(struct shell_ctx *ctx, int argc, char **argv) {
    (void)ctx;
    const char *usage = \
        "usage: net_ipv4_set_subnet <dev_num> <subnet>\n"
        "  dev_num        device number as returned by net_add_device\n"
        "  subnet         a.b.c.d\n";
    if (argc != 3) {
        fputs(usage, ctx->fout);
        return;
    }
    uint32_t dev_num = strtoul(argv[1], NULL, 0);
    char *subnet_str = argv[2];
    uint32_t subnet = inet_addr(subnet_str);
    COVERAGE_ON();
    f76ret_t r = umka_sys_net_ipv4_set_subnet(dev_num, subnet);
    COVERAGE_OFF();
    if (r.eax == UINT32_MAX) {
        fprintf(ctx->fout, "status: fail\n");
    } else {
        fprintf(ctx->fout, "status: ok\n");
    }
}

static void
cmd_net_ipv4_get_gw(struct shell_ctx *ctx, int argc, char **argv) {
    (void)ctx;
    const char *usage = \
        "usage: net_ipv4_get_gw <dev_num>\n"
        "  dev_num        device number as returned by net_add_device\n";
    if (argc != 2) {
        fputs(usage, ctx->fout);
        return;
    }
    uint32_t dev_num = strtoul(argv[1], NULL, 0);
    COVERAGE_ON();
    f76ret_t r = umka_sys_net_ipv4_get_gw(dev_num);
    COVERAGE_OFF();
    if (r.eax == UINT32_MAX) {
        fprintf(ctx->fout, "status: fail\n");
    } else {
        fprintf(ctx->fout, "%d.%d.%d.%d\n",
               (uint8_t)(r.eax >>  0), (uint8_t)(r.eax >>  8),
               (uint8_t)(r.eax >> 16), (uint8_t)(r.eax >> 24));
    }
}

static void
cmd_net_ipv4_set_gw(struct shell_ctx *ctx, int argc, char **argv) {
    (void)ctx;
    const char *usage = \
        "usage: net_ipv4_set_gw <dev_num> <gw>\n"
        "  dev_num        device number as returned by net_add_device\n"
        "  gw             a.b.c.d\n";
    if (argc != 3) {
        fputs(usage, ctx->fout);
        return;
    }
    uint32_t dev_num = strtoul(argv[1], NULL, 0);
    char *gw_str = argv[2];
    uint32_t gw = inet_addr(gw_str);
    COVERAGE_ON();
    f76ret_t r = umka_sys_net_ipv4_set_gw(dev_num, gw);
    COVERAGE_OFF();
    if (r.eax == UINT32_MAX) {
        fprintf(ctx->fout, "status: fail\n");
    } else {
        fprintf(ctx->fout, "status: ok\n");
    }
}

static void
cmd_net_arp_get_count(struct shell_ctx *ctx, int argc, char **argv) {
    (void)ctx;
    const char *usage = \
        "usage: net_arp_get_count <dev_num>\n"
        "  dev_num        device number as returned by net_add_device\n";
    if (argc != 2) {
        fputs(usage, ctx->fout);
        return;
    }
    uint32_t dev_num = strtoul(argv[1], NULL, 0);
    COVERAGE_ON();
    f76ret_t r = umka_sys_net_arp_get_count(dev_num);
    COVERAGE_OFF();
    if (r.eax == UINT32_MAX) {
        fprintf(ctx->fout, "status: fail\n");
    } else {
        fprintf(ctx->fout, "%" PRIi32 "\n", r.eax);
    }
}

static void
cmd_net_arp_get_entry(struct shell_ctx *ctx, int argc, char **argv) {
    (void)ctx;
    const char *usage = \
        "usage: net_arp_get_entry <dev_num> <arp_num>\n"
        "  dev_num        device number as returned by net_add_device\n"
        "  arp_num        arp number as returned by net_add_device\n";
    if (argc != 3) {
        fputs(usage, ctx->fout);
        return;
    }
    uint32_t dev_num = strtoul(argv[1], NULL, 0);
    uint32_t arp_num = strtoul(argv[2], NULL, 0);
    arp_entry_t arp;
    COVERAGE_ON();
    f76ret_t r = umka_sys_net_arp_get_entry(dev_num, arp_num, &arp);
    COVERAGE_OFF();
    if (r.eax == UINT32_MAX) {
        fprintf(ctx->fout, "status: fail\n");
    } else {
        fprintf(ctx->fout, "arp #%u: IP %d.%d.%d.%d, "
               "mac %2.2" SCNu8 ":%2.2" SCNu8 ":%2.2" SCNu8
               ":%2.2" SCNu8 ":%2.2" SCNu8 ":%2.2" SCNu8 ", "
               "status %" PRIu16 ", "
               "ttl %" PRIu16 "\n",
               arp_num,
               (uint8_t)(arp.ip >>  0), (uint8_t)(arp.ip >>  8),
               (uint8_t)(arp.ip >> 16), (uint8_t)(arp.ip >> 24),
               arp.mac[0], arp.mac[1], arp.mac[2],
               arp.mac[3], arp.mac[4], arp.mac[5],
               arp.status, arp.ttl);
    }
}

static void
cmd_net_arp_add_entry(struct shell_ctx *ctx, int argc, char **argv) {
    (void)ctx;
    const char *usage = \
        "usage: net_arp_add_entry <dev_num> <addr> <mac> <status> <ttl>\n"
        "  dev_num        device number as returned by net_add_device\n"
        "  addr           IP addr\n"
        "  mac            ethernet addr\n"
        "  status         see ARP.inc\n"
        "  ttl            Time to live\n";
    if (argc != 6) {
        fputs(usage, ctx->fout);
        return;
    }
    arp_entry_t arp;
    uint32_t dev_num = strtoul(argv[1], NULL, 0);
    arp.ip = inet_addr(argv[2]);
    sscanf(argv[3], "%" SCNu8 ":%" SCNu8 ":%" SCNu8
                    ":%" SCNu8 ":%" SCNu8 ":%" SCNu8,
                    arp.mac+0, arp.mac+1, arp.mac+2,
                    arp.mac+3, arp.mac+4, arp.mac+5);
    arp.status = strtoul(argv[4], NULL, 0);
    arp.ttl = strtoul(argv[5], NULL, 0);
    COVERAGE_ON();
    f76ret_t r = umka_sys_net_arp_add_entry(dev_num, &arp);
    COVERAGE_OFF();
    if (r.eax == UINT32_MAX) {
        fprintf(ctx->fout, "status: fail\n");
    }
}

static void
cmd_net_arp_del_entry(struct shell_ctx *ctx, int argc, char **argv) {
    (void)ctx;
    const char *usage = \
        "usage: net_arp_del_entry <dev_num> <arp_num>\n"
        "  dev_num        device number as returned by net_add_device\n"
        "  arp_num        arp number as returned by net_add_device\n";
    if (argc != 3) {
        fputs(usage, ctx->fout);
        return;
    }
    uint32_t dev_num = strtoul(argv[1], NULL, 0);
    int32_t arp_num = strtoul(argv[2], NULL, 0);
    COVERAGE_ON();
    f76ret_t r = umka_sys_net_arp_del_entry(dev_num, arp_num);
    COVERAGE_OFF();
    if (r.eax == UINT32_MAX) {
        fprintf(ctx->fout, "status: fail\n");
    }
}

static void
cmd_osloop(struct shell_ctx *ctx, int argc, char **argv) {
    (void)ctx;
    (void)argv;
    const char *usage = \
        "usage: osloop\n";
    if (argc != 1) {
        fputs(usage, ctx->fout);
        return;
    }
    COVERAGE_ON();
    umka_osloop();
    COVERAGE_OFF();
}

static void
cmd_bg_set_size(struct shell_ctx *ctx, int argc, char **argv) {
    (void)ctx;
    const char *usage = \
        "usage: bg_set_size <xsize> <ysize>\n"
        "  xsize          in pixels\n"
        "  ysize          in pixels\n";
    if (argc != 3) {
        fputs(usage, ctx->fout);
        return;
    }
    uint32_t xsize = strtoul(argv[1], NULL, 0);
    uint32_t ysize = strtoul(argv[2], NULL, 0);
    COVERAGE_ON();
    umka_sys_bg_set_size(xsize, ysize);
    COVERAGE_OFF();
}

static void
cmd_bg_put_pixel(struct shell_ctx *ctx, int argc, char **argv) {
    (void)ctx;
    const char *usage = \
        "usage: bg_put_pixel <offset> <color>\n"
        "  offset         in bytes, (x+y*xsize)*3\n"
        "  color          in hex\n";
    if (argc != 3) {
        fputs(usage, ctx->fout);
        return;
    }
    size_t offset = strtoul(argv[1], NULL, 0);
    uint32_t color = strtoul(argv[2], NULL, 0);
    COVERAGE_ON();
    umka_sys_bg_put_pixel(offset, color);
    COVERAGE_OFF();
}

static void
cmd_bg_redraw(struct shell_ctx *ctx, int argc, char **argv) {
    (void)ctx;
    (void)argv;
    const char *usage = \
        "usage: bg_redraw\n";
    if (argc != 1) {
        fputs(usage, ctx->fout);
        return;
    }
    COVERAGE_ON();
    umka_sys_bg_redraw();
    COVERAGE_OFF();
}

static void
cmd_bg_set_mode(struct shell_ctx *ctx, int argc, char **argv) {
    (void)ctx;
    const char *usage = \
        "usage: bg_set_mode <mode>\n"
        "  mode           1 = tile, 2 = stretch\n";
    if (argc != 3) {
        fputs(usage, ctx->fout);
        return;
    }
    uint32_t mode = strtoul(argv[1], NULL, 0);
    COVERAGE_ON();
    umka_sys_bg_set_mode(mode);
    COVERAGE_OFF();
}

static void
cmd_bg_put_img(struct shell_ctx *ctx, int argc, char **argv) {
    (void)ctx;
    const char *usage = \
        "usage: bg_put_img <image> <offset>\n"
        "  image          file\n"
        "  offset         in bytes, (x+y*xsize)*3\n";
    if (argc != 4) {
        fputs(usage, ctx->fout);
        return;
    }
    FILE *f = fopen(argv[1], "rb");
    fseek(f, 0, SEEK_END);
    size_t fsize = ftell(f);
    rewind(f);
    uint8_t *image = (uint8_t*)malloc(fsize);
    fread(image, fsize, 1, f);
    fclose(f);
    size_t offset = strtoul(argv[2], NULL, 0);
    COVERAGE_ON();
    umka_sys_bg_put_img(image, offset, fsize);
    COVERAGE_OFF();
}

static void
cmd_board_get(struct shell_ctx *ctx, int argc, char **argv) {
    (void)argv;
    const char *usage = \
        "usage: board_get [<-l|-a>] [-n]\n";
    if (argc > 2) {
        fputs(usage, ctx->fout);
        return;
    }
    optparse_init(&ctx->opts, argv);
    int flush = 0;
    int line = 0;
    int force_newline = 0;
    int opt;
    while ((opt = optparse(&ctx->opts, "fln")) != -1) {
        switch (opt) {
        case 'f':
            flush = 1;
            __attribute__((fallthrough));   // TODO: use [[fallthrough]]; (C23)
        case 'l':
            line = 1;
            break;
        case 'n':
            force_newline = 1;
            break;
        default:
            fputs(usage, ctx->fout);
            return;
        }
    }
    if (!line) {
        COVERAGE_ON();
        struct board_get_ret c = umka_sys_board_get();
        COVERAGE_OFF();
        if (!c.status) {
            fprintf(ctx->fout, "(empty)\n");
        } else {
            fprintf(ctx->fout, "%c\n", c.value);
        }
    } else {
        struct board_get_ret c;
        do {
            COVERAGE_ON();
            c = umka_sys_board_get();
            COVERAGE_OFF();
            if (c.status && (c.value != '\r')) {
                fputc(c.value, ctx->fout);
            }
        } while (c.status && ((c.value != '\n') || flush));
        if (force_newline) {
            fputc('\n', ctx->fout);
        }
    }
}

static void
cmd_board_put(struct shell_ctx *ctx, int argc, char **argv) {
    (void)argv;
    const char *usage = \
        "usage: board_put <char>\n";
    if (argc != 2) {
        fputs(usage, ctx->fout);
        return;
    }
    char c = argv[1][0];
    if (c == '\\') {
        switch ((c = argv[1][1])) {
        case 'n':
            c = '\n';
            break;
        case 'r':
            c = '\r';
            break;
        case 't':
            c = '\t';
            break;
        default:
            fprintf(ctx->fout, "unknown escape symbol: '%c'\n", c);
            c = '?';
            break;
        }
    }
    COVERAGE_ON();
    umka_sys_board_put(c);
    COVERAGE_OFF();
}

static void
cmd_bg_map(struct shell_ctx *ctx, int argc, char **argv) {
    (void)argv;
    const char *usage = \
        "usage: bg_map\n";
    if (argc != 1) {
        fputs(usage, ctx->fout);
        return;
    }
    COVERAGE_ON();
    void *addr = umka_sys_bg_map();
    COVERAGE_OFF();
    fprintf(ctx->fout, "%p\n", addr);
}

static void
cmd_bg_unmap(struct shell_ctx *ctx, int argc, char **argv) {
    (void)ctx;
    const char *usage = \
        "usage: bg_unmap <addr>\n"
        "  addr           return value of bg_map\n";
    if (argc != 2) {
        fputs(usage, ctx->fout);
        return;
    }
    void *addr = (void*)strtoul(argv[1], NULL, 0);
    COVERAGE_ON();
    uint32_t status = umka_sys_bg_unmap(addr);
    COVERAGE_OFF();
    fprintf(ctx->fout, "status = %d\n", status);
}

static void
cmd_help(struct shell_ctx *ctx, int argc, char **argv);

func_table_t cmd_cmds[] = {
    { "send_scancode",                  cmd_send_scancode },
    { "umka_boot",                      cmd_umka_boot },
    { "umka_set_boot_params",           cmd_umka_set_boot_params },
    { "acpi_call",                      cmd_acpi_call },
    { "acpi_enable",                    cmd_acpi_enable },
    { "acpi_get_usage",                 cmd_acpi_get_usage },
    { "acpi_preload_table",             cmd_acpi_preload_table },
    { "acpi_set_usage",                 cmd_acpi_set_usage },
    { "acpi_get_node_alloc_cnt",        cmd_acpi_get_node_alloc_cnt },
    { "acpi_get_node_free_cnt",         cmd_acpi_get_node_free_cnt },
    { "acpi_get_node_cnt",              cmd_acpi_get_node_cnt },
    { "board_get",                      cmd_board_get },
    { "board_put",                      cmd_board_put },
    { "bg_map",                         cmd_bg_map },
    { "bg_put_img",                     cmd_bg_put_img },
    { "bg_put_pixel",                   cmd_bg_put_pixel },
    { "bg_redraw",                      cmd_bg_redraw },
    { "bg_set_mode",                    cmd_bg_set_mode },
    { "bg_set_size",                    cmd_bg_set_size },
    { "bg_unmap",                       cmd_bg_unmap },
    { "blit_bitmap",                    cmd_blit_bitmap },
    { "button",                         cmd_button },
    { "cd",                             cmd_cd },
    { "set",                            cmd_set },
    { "get",                            cmd_get },
    { "get_key",                        cmd_get_key },
    { "disk_add",                       cmd_disk_add },
    { "disk_del",                       cmd_disk_del },
    { "display_number",                 cmd_display_number },
    { "draw_line",                      cmd_draw_line },
    { "draw_rect",                      cmd_draw_rect },
    { "draw_window",                    cmd_draw_window },
    { "dump_appdata",                   cmd_dump_appdata },
    { "dump_key_buff",                  cmd_dump_key_buff },
    { "dump_wdata",                     cmd_dump_wdata },
    { "dump_win_pos",                   cmd_dump_win_pos },
    { "dump_win_stack",                 cmd_dump_win_stack },
    { "dump_win_map",                   cmd_dump_win_map },
    { "exec",                           cmd_exec },
    { "get_font_size",                  cmd_get_font_size },
    { "get_font_smoothing",             cmd_get_font_smoothing },
    { "get_keyboard_lang",              cmd_get_keyboard_lang },
    { "get_keyboard_layout",            cmd_get_keyboard_layout },
    { "get_keyboard_mode",              cmd_get_keyboard_mode },
    { "get_mouse_buttons_state",        cmd_get_mouse_buttons_state },
    { "get_mouse_buttons_state_events", cmd_get_mouse_buttons_state_events },
    { "get_mouse_pos_screen",           cmd_get_mouse_pos_screen },
    { "get_mouse_pos_window",           cmd_get_mouse_pos_window },
    { "get_screen_area",                cmd_get_screen_area },
    { "get_screen_size",                cmd_get_screen_size },
    { "get_skin_height",                cmd_get_skin_height },
    { "get_skin_margins",               cmd_get_skin_margins },
    { "get_system_lang",                cmd_get_system_lang },
    { "get_window_colors",              cmd_get_window_colors },
    { "help",                           cmd_help },
    { "i40",                            cmd_i40 },
    // f68
    { "kos_sys_misc_init_heap",         cmd_kos_sys_misc_init_heap },   // 11
    { "kos_sys_misc_load_file",         cmd_kos_sys_misc_load_file },   // 27
    { "load_cursor_from_file",          cmd_load_cursor_from_file },
    { "load_cursor_from_mem",           cmd_load_cursor_from_mem },
    { "load_dll",                       cmd_load_dll },
    { "ls70",                           cmd_ls70 },
    { "ls80",                           cmd_ls80 },
    { "move_window",                    cmd_move_window },
    { "mouse_move",                     cmd_mouse_move },
    { "stack_init",                     cmd_stack_init },
    { "net_accept",                     cmd_net_accept },
    { "net_add_device",                 cmd_net_add_device },
    { "net_arp_add_entry",              cmd_net_arp_add_entry },
    { "net_arp_del_entry",              cmd_net_arp_del_entry },
    { "net_arp_get_count",              cmd_net_arp_get_count },
    { "net_arp_get_entry",              cmd_net_arp_get_entry },
    { "net_bind",                       cmd_net_bind },
    { "net_close_socket",               cmd_net_close_socket },
    { "net_connect",                    cmd_net_connect },
    { "net_dev_reset",                  cmd_net_dev_reset },
    { "net_dev_stop",                   cmd_net_dev_stop },
    { "net_eth_read_mac",               cmd_net_eth_read_mac },
    { "net_get_byte_rx_count",          cmd_net_get_byte_rx_count },
    { "net_get_byte_tx_count",          cmd_net_get_byte_tx_count },
    { "net_get_dev",                    cmd_net_get_dev },
    { "net_get_dev_count",              cmd_net_get_dev_count },
    { "net_get_dev_name",               cmd_net_get_dev_name },
    { "net_get_dev_type",               cmd_net_get_dev_type },
    { "net_get_link_status",            cmd_net_get_link_status },
    { "net_get_packet_rx_count",        cmd_net_get_packet_rx_count },
    { "net_get_packet_tx_count",        cmd_net_get_packet_tx_count },
    { "net_ipv4_get_addr",              cmd_net_ipv4_get_addr },
    { "net_ipv4_get_dns",               cmd_net_ipv4_get_dns },
    { "net_ipv4_get_gw",                cmd_net_ipv4_get_gw },
    { "net_ipv4_get_subnet",            cmd_net_ipv4_get_subnet },
    { "net_ipv4_set_addr",              cmd_net_ipv4_set_addr },
    { "net_ipv4_set_dns",               cmd_net_ipv4_set_dns },
    { "net_ipv4_set_gw",                cmd_net_ipv4_set_gw },
    { "net_ipv4_set_subnet",            cmd_net_ipv4_set_subnet },
    { "net_listen",                     cmd_net_listen },
    { "net_open_socket",                cmd_net_open_socket },
    { "osloop",                         cmd_osloop },
    { "pci_get_path",                   cmd_pci_get_path },
    { "pci_set_path",                   cmd_pci_set_path },
    { "process_info",                   cmd_process_info },
    { "put_image",                      cmd_put_image },
    { "put_image_palette",              cmd_put_image_palette },
    { "pwd",                            cmd_pwd },
    { "ramdisk_init",                   cmd_ramdisk_init },
    { "read70",                         cmd_read70 },
    { "read80",                         cmd_read80 },
    { "scrot",                          cmd_scrot },
    { "write_devices_dat",              cmd_write_devices_dat },
    { "set_button_style",               cmd_set_button_style },
    { "set_cursor",                     cmd_set_cursor },
    { "set_cwd",                        cmd_cd },
    { "set_font_size",                  cmd_set_font_size },
    { "set_font_smoothing",             cmd_set_font_smoothing },
    { "set_keyboard_lang",              cmd_set_keyboard_lang },
    { "set_keyboard_layout",            cmd_set_keyboard_layout },
    { "set_keyboard_mode",              cmd_set_keyboard_mode },
    { "set_mouse_pos_screen",           cmd_set_mouse_pos_screen },
    { "set_pixel",                      cmd_set_pixel },
    { "set_screen_area",                cmd_set_screen_area },
    { "set_skin",                       cmd_set_skin },
    { "set_system_lang",                cmd_set_system_lang },
    { "set_window_caption",             cmd_set_window_caption },
    { "set_window_colors",              cmd_set_window_colors },
    { "csleep",                         cmd_csleep },
    { "stat70",                         cmd_stat70 },
    { "stat80",                         cmd_stat80 },
    { "var",                            cmd_var },
    { "check_for_event",                cmd_check_for_event },
    { "wait_for_idle",                  cmd_wait_for_idle },
    { "wait_for_os_idle",               cmd_wait_for_os_idle },
    { "wait_for_window",                cmd_wait_for_window },
    { "window_redraw",                  cmd_window_redraw },
    { "write_text",                     cmd_write_text },
    { "switch_to_thread",               cmd_switch_to_thread },
    { "new_sys_thread",                 cmd_new_sys_thread },
    { NULL,                             NULL },
};

static void
shell_run_cmd_sync(struct shell_ctx *ctx) {
    struct umka_cmd *cmd = umka_cmd_buf;
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
    case UMKA_CMD_SYS_LFN: {
        struct cmd_sys_lfn_arg *c = &cmd->sys_lfn.arg;
        COVERAGE_ON();
        umka_sys_lfn(c->bufptr, c->r, c->f70or80);
        COVERAGE_OFF();
        break;
        }
    default:
        fprintf(ctx->fout, "[!] unknown command: %u\n", cmd->type);
        break;
    }
    atomic_store_explicit(&cmd->status, SHELL_CMD_STATUS_DONE,
                          memory_order_release);
    pthread_cond_signal(&ctx->cmd_done);
}

struct umka_cmd *
shell_get_cmd(struct shell_ctx *shell) {
    (void)shell;
    return umka_cmd_buf;
}

void
shell_run_cmd(struct shell_ctx *ctx) {
    struct umka_cmd *cmd = umka_cmd_buf;
    atomic_store_explicit(&cmd->status, SHELL_CMD_STATUS_READY,
                          memory_order_release);
    if (atomic_load_explicit(ctx->running, memory_order_acquire) == UMKA_RUNNING_YES) {
        pthread_cond_wait(&ctx->cmd_done, &ctx->cmd_mutex);
    } else {
        shell_run_cmd_sync(ctx);
    }
}

void
shell_clear_cmd(struct umka_cmd *cmd) {
    atomic_store_explicit(&cmd->status, SHELL_CMD_STATUS_EMPTY,
                          memory_order_release);
}

static void
cmd_help(struct shell_ctx *ctx, int argc, char **argv) {
    const char *usage = \
        "usage: help [command]\n"
        "  command        help on this command usage\n";
    switch (argc) {
    case 1:
        fputs(usage, ctx->fout);
        puts("\navailable commands:\n");
        for (func_table_t *ft = cmd_cmds; ft->name; ft++) {
            fprintf(ctx->fout, "  %s\n", ft->name);
        }
        break;
    case 2: {
        const char *cmd_name = argv[1];
        for (func_table_t *ft = cmd_cmds; ft->name; ft++) {
            if (!strcmp(ft->name, cmd_name)) {
                ft->func(ctx, 0, NULL);
                return;
            }
        }
        fprintf(ctx->fout, "no such command: %s\n", cmd_name);
        break;
    }
    default:
        fputs(usage, ctx->fout);
        return;
    }
}

void *
run_test(struct shell_ctx *ctx) {
    int fdstdin = -1;
    if (ctx->fin != stdin) {
        fdstdin = dup(STDIN_FILENO);
        close(STDIN_FILENO);
        dup2(fileno(ctx->fin), STDIN_FILENO);
        fclose(ctx->fin);
    }
    ic_init_custom_malloc(NULL, NULL, NULL);
//    ic_style_def("ic-prompt","ansi-default");
    ic_enable_color(0);
    ic_set_prompt_marker(NULL, NULL);
    ic_enable_multiline(0);
    ic_enable_beep(0);

    pthread_mutex_lock(&ctx->cmd_mutex);
    int is_tty = isatty(fileno(stdin));
    char **argv = (char**)calloc(MAX_COMMAND_ARGS + 1, sizeof(char*));
    ic_set_default_completer(completer, NULL);
    ic_set_default_highlighter(highlighter, NULL);
    ic_enable_auto_tab(1);
    sprintf(prompt_line, "%s", last_dir);
    char *line;
    while ((line = ic_readline(prompt_line)) || !feof(stdin)) {
        if (!is_tty) {
            prompt(ctx);
            fprintf(ctx->fout, "%s\n", line ? line : "");
        }
        if (!line) {
            continue;
        }
        if (line[0] == '\0') {
            break;
        }
        if (!strcmp(line, "X") || !strcmp(line, "q")) {
            free(line);
            break;
        } else if (line[0] == '\0' || line[0] == '#' || line[0] == '\n'
                   || *line == '\r') {
            free(line);
            continue;
        } else {
            int argc = split_args(line, argv);
            func_table_t *ft;
            for (ft = cmd_cmds; ft->name; ft++) {
                if (!strcmp(argv[0], ft->name)) {
                    break;
                }
            }
            if (ft->name) {
                ft->func(ctx, argc, argv);
            } else {
                fprintf(ctx->fout, "unknown command: %s\n", argv[0]);
            }
            free(line);
        }
    }
    free(argv);

    pthread_mutex_unlock(&ctx->cmd_mutex);

    if (fdstdin != -1) {
        close(STDIN_FILENO);
        dup2(fdstdin, STDIN_FILENO);
        close(fdstdin);
    }
    return NULL;
}

struct shell_ctx *
shell_init(const int reproducible, const char *hist_file,
           const struct umka_ctx *umka, const struct umka_io *io, FILE *fin) {
    struct shell_ctx *ctx = malloc(sizeof(struct shell_ctx));
    ctx->umka = umka;
    ctx->io = io;
    ctx->reproducible = reproducible;
    ctx->hist_file = hist_file;
    ctx->var = NULL;
    ctx->fin = fin;
    ctx->fout = stdout;
    ctx->running = &umka->running;
    pthread_cond_init(&ctx->cmd_done, NULL);
    pthread_mutex_init(&ctx->cmd_mutex, NULL);
    return ctx;
}

void
shell_close(struct shell_ctx *ctx) {
    struct shell_var *next;
    for (struct shell_var *var = ctx->var; var; var = next) {
        next = var->next;
        free(var);
    }
}

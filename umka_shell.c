/*
    umka_shell: User-Mode KolibriOS developer tools, the shell
    Copyright (C) 2018--2020  Ivan Baravy <dunkaist@gmail.com>

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

#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <inttypes.h>
#include <limits.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <assert.h>
#include <time.h>
#include "vdisk.h"
#include "kolibri.h"
#include "syscalls.h"
#include "trace.h"
#include "lodepng.h"

#define PATH_MAX 4096
#define FGETS_BUF_LEN 4096
#define MAX_COMMAND_ARGS 42
#define PRINT_BYTES_PER_LINE 32
#define MAX_DIRENTS_TO_READ 100
#define MAX_BYTES_TO_READ (1024*1024)

#define DEFAULT_READDIR_ENCODING UTF8
#define DEFAULT_PATH_ENCODING UTF8

diskfunc_t vdisk_functions = {
                                 .strucsize = sizeof(diskfunc_t),
                                 .close = vdisk_close,
                                 .closemedia = NULL,
                                 .querymedia = vdisk_querymedia,
                                 .read = vdisk_read,
                                 .write = vdisk_write,
                                 .flush = NULL,
                                 .adjust_cache_size = NULL,
                                };
char cur_dir[PATH_MAX] = "/";
const char *last_dir = cur_dir;
bool cur_dir_changed = true;

char cmd_buf[FGETS_BUF_LEN];

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

const char *get_f70_status_name(int s) {
    switch (s) {
    case ERROR_SUCCESS:
//        return "";
    case ERROR_DISK_BASE:
    case ERROR_UNSUPPORTED_FS:
    case ERROR_UNKNOWN_FS:
    case ERROR_PARTITION:
    case ERROR_FILE_NOT_FOUND:
    case ERROR_END_OF_FILE:
    case ERROR_MEMORY_POINTER:
    case ERROR_DISK_FULL:
    case ERROR_FS_FAIL:
    case ERROR_ACCESS_DENIED:
    case ERROR_DEVICE:
    case ERROR_OUT_OF_MEMORY:
        return f70_status_name[s];
    default:
        return "unknown";
    }
}

void convert_f70_file_attr(uint32_t attr, char s[KF_ATTR_CNT+1]) {
    s[0] = (attr & KF_READONLY) ? 'r' : '-';
    s[1] = (attr & KF_HIDDEN)   ? 'h' : '-';
    s[2] = (attr & KF_SYSTEM)   ? 's' : '-';
    s[3] = (attr & KF_LABEL)    ? 'l' : '-';
    s[4] = (attr & KF_FOLDER)   ? 'f' : '-';
    s[5] = '\0';
}

void print_f70_status(f7080ret_t *r, int use_ebx) {
    printf("status = %d %s", r->status, get_f70_status_name(r->status));
    if (use_ebx && (r->status == ERROR_SUCCESS || r->status == ERROR_END_OF_FILE))
        printf(", count = %d", r->count);
    putchar('\n');
}

bool parse_uintmax(const char *str, uintmax_t *res) {
    char *endptr;
    *res = strtoumax(str, &endptr, 0);
    bool ok = (str != endptr) && (*endptr == '\0');
    return ok;
}

bool parse_uint32(const char *str, uint32_t *res) {
    uintmax_t x;
    if (parse_uintmax(str, &x) && x <= UINT32_MAX) {
        *res = (uint32_t)x;
        return true;
    } else {
        perror("invalid number");
        return false;
    }
}

bool parse_uint64(const char *str, uint64_t *res) {
    uintmax_t x;
    if (parse_uintmax(str, &x) && x <= UINT64_MAX) {
        *res = x;
        return true;
    } else {
        perror("invalid number");
        return false;
    }
}

void print_bytes(uint8_t *x, size_t len) {
    for (size_t i = 0; i < len; i++) {
        if (i % PRINT_BYTES_PER_LINE == 0 && i != 0) {
            printf("\n");
        }
        printf("%2.2x", x[i]);
    }
    putchar('\n');
}

void print_hash(uint8_t *x, size_t len) {
    hash_context ctx;
    hash_oneshot(&ctx, x, len);
    for (size_t i = 0; i < HASH_SIZE; i++) {
        printf("%2.2x", ctx.hash[i]);
    }
    putchar('\n');
}

const char *get_last_dir(const char *path) {
    const char *last = strrchr(path, '/');
    if (!last) {
        last = path;
    } else if (last != path || last[1] != '\0') {
        last++;
    }
    return last;
}

void prompt() {
    if (cur_dir_changed) {
        COVERAGE_ON();
        umka_sys_get_cwd(cur_dir, PATH_MAX);
        COVERAGE_OFF();
        last_dir = get_last_dir(cur_dir);
        cur_dir_changed = false;
    }
    printf("%s> ", last_dir);
    fflush(stdout);
}

int next_line(FILE *file, int is_tty) {
    if (is_tty) {
        prompt();
    }
    return fgets(cmd_buf, FGETS_BUF_LEN, file) != NULL;
}

int split_args(char *s, char **argv) {
    int argc = -1;
    for (; (argv[++argc] = strtok(s, " \t\n")) != NULL; s = NULL);
    return argc;
}

void shell_disk_list_partitions(disk_t *d) {
    for (size_t i = 0; i < d->num_partitions; i++) {
        printf("/%s/%d: ", d->name, i+1);
        if (d->partitions[i]->fs_user_functions == xfs_user_functions) {
            printf("xfs\n");
        } else if (d->partitions[i]->fs_user_functions == ext_user_functions) {
            printf("ext\n");
        } else if (d->partitions[i]->fs_user_functions == fat_user_functions) {
            printf("fat\n");
        } else if (d->partitions[i]->fs_user_functions == ntfs_user_functions) {
            printf("ntfs\n");
        } else {
            printf("???\n");
        }
    }
}

void shell_disk_add(int argc, char **argv) {
    const char *usage = \
        "usage: disk_add <file> <name> [option]...\n"
        "  <file>           absolute or relative path\n"
        "  <name>           disk name, e.g. hd0 or rd\n"
        "  -c cache size    size of disk cache in bytes";
    if (argc < 3) {
        puts(usage);
        return;
    }
    unsigned cache_size = 0u;
    int opt;
    optind = 1;
    const char *file_name = argv[optind++];
    const char *disk_name = argv[optind++];
    while ((opt = getopt(argc, argv, "c:")) != -1) {
        switch (opt) {
        case 'c':
            cache_size = strtoul(optarg, NULL, 0);
            vdisk_functions.adjust_cache_size = vdisk_adjust_cache_size;
            break;
        default:
            puts(usage);
            return;
        }
    }

    void *userdata = vdisk_init(file_name, cache_size);
    if (userdata) {
        COVERAGE_ON();
        void *vdisk = disk_add(&vdisk_functions, disk_name, userdata, 0);
        COVERAGE_OFF();
        if (vdisk) {
            COVERAGE_ON();
            disk_media_changed(vdisk, 1);
            COVERAGE_OFF();
            shell_disk_list_partitions(vdisk);
            return;
        }
    }
    printf("umka: can't add file '%s' as disk '%s'\n", file_name, disk_name);
    return;
}

void shell_disk_del_by_name(const char *name) {
    for(disk_t *d = disk_list.next; d != &disk_list; d = d->next) {
        if (!strcmp(d->name, name)) {
            COVERAGE_ON();
            disk_del(d);
            COVERAGE_OFF();
            return;
        }
    }
    printf("umka: can't find disk '%s'\n", name);
}

void shell_disk_del(int argc, char **argv) {
    (void)argc;
    const char *name = argv[1];
    shell_disk_del_by_name(name);
    return;
}

void shell_pwd(int argc, char **argv) {
    (void)argc;
    (void)argv;
    bool quoted = false;
    const char *quote = quoted ? "'" : "";
    COVERAGE_ON();
    umka_sys_get_cwd(cur_dir, PATH_MAX);
    COVERAGE_OFF();
    printf("%s%s%s\n", quote, cur_dir, quote);
}

void shell_set_pixel(int argc, char **argv) {
    size_t x = strtoul(argv[1], NULL, 0);
    size_t y = strtoul(argv[2], NULL, 0);
    uint32_t color = strtoul(argv[3], NULL, 16);
    int invert = (argc == 5) && !strcmp(argv[4], "-i");
    COVERAGE_ON();
    umka_sys_set_pixel(x, y, color, invert);
    COVERAGE_OFF();
}

void shell_write_text(int argc, char **argv) {
    (void)argc;
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
    umka_sys_write_text(x, y, color, asciiz, fill_background, font_and_encoding, draw_to_buffer, scale_factor, string, length, background_color_or_buffer);
    COVERAGE_OFF();
}

void shell_dump_win_stack(int argc, char **argv) {
    int depth = 5;
    if (argc > 1) {
        depth = strtol(argv[1], NULL, 0);
    }
    for (int i = 0; i < depth; i++) {
        printf("%3i: %3u\n", i, kos_win_stack[i]);
    }
}

void shell_dump_win_pos(int argc, char **argv) {
    int depth = 5;
    if (argc > 1) {
        depth = strtol(argv[1], NULL, 0);
    }
    for (int i = 0; i < depth; i++) {
        printf("%3i: %3u\n", i, kos_win_pos[i]);
    }
}

void shell_process_info(int argc, char **argv) {
    (void)argc;
    process_information_t info;
    int32_t pid = strtol(argv[1], NULL, 0);
    COVERAGE_ON();
    umka_sys_process_info(pid, &info);
    COVERAGE_OFF();
    printf("cpu_usage: %u\n", info.cpu_usage);
    printf("window_stack_position: %u\n", info.window_stack_position);
    printf("window_stack_value: %u\n", info.window_stack_value);
    printf("process_name: %s\n", info.process_name);
    printf("memory_start: 0x%.8" PRIx32 "\n", info.memory_start);
    printf("used_memory: %u (0x%x)\n", info.used_memory, info.used_memory);
    printf("pid: %u\n", info.pid);
    printf("box: %u %u %u %u\n", info.box.left, info.box.top, info.box.width, info.box.height);
    printf("slot_state: %u\n", info.slot_state);
    printf("client_box: %u %u %u %u\n", info.client_box.left, info.client_box.top, info.client_box.width, info.client_box.height);
    printf("wnd_state: 0x%.2" PRIx8 "\n", info.wnd_state);
}

void shell_display_number(int argc, char **argv) {
    (void)argc;
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
    umka_sys_display_number(is_pointer, base, digits_to_display, is_qword, show_leading_zeros, number_or_pointer, x, y, color, fill_background, font, draw_to_buffer, scale_factor, background_color_or_buffer);
    COVERAGE_OFF();
}

void shell_set_window_colors(int argc, char **argv) {
    if (argc != (1 + sizeof(system_colors_t)/4)) {
        printf("10 colors required\n");
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

void shell_get_window_colors(int argc, char **argv) {
    (void)argc;
    (void)argv;
    system_colors_t colors;
    COVERAGE_ON();
    umka_sys_get_window_colors(&colors);
    COVERAGE_OFF();
    printf("0x%.8" PRIx32 " frame\n", colors.frame);
    printf("0x%.8" PRIx32 " grab\n", colors.grab);
    printf("0x%.8" PRIx32 " work_3d_dark\n", colors.work_3d_dark);
    printf("0x%.8" PRIx32 " work_3d_light\n", colors.work_3d_light);
    printf("0x%.8" PRIx32 " grab_text\n", colors.grab_text);
    printf("0x%.8" PRIx32 " work\n", colors.work);
    printf("0x%.8" PRIx32 " work_button\n", colors.work_button);
    printf("0x%.8" PRIx32 " work_button_text\n", colors.work_button_text);
    printf("0x%.8" PRIx32 " work_text\n", colors.work_text);
    printf("0x%.8" PRIx32 " work_graph\n", colors.work_graph);
}

void shell_get_skin_height(int argc, char **argv) {
    (void)argc;
    (void)argv;
    COVERAGE_ON();
    uint32_t skin_height = umka_sys_get_skin_height();
    COVERAGE_OFF();
    printf("%" PRIu32 "\n", skin_height);
}

void shell_get_screen_area(int argc, char **argv) {
    (void)argc;
    (void)argv;
    rect_t wa;
    COVERAGE_ON();
    umka_sys_get_screen_area(&wa);
    COVERAGE_OFF();
    printf("%" PRIu32 " left\n", wa.left);
    printf("%" PRIu32 " top\n", wa.top);
    printf("%" PRIu32 " right\n", wa.right);
    printf("%" PRIu32 " bottom\n", wa.bottom);
}

void shell_set_screen_area(int argc, char **argv) {
    if (argc != 5) {
        printf("left top right bottom\n");
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

void shell_get_skin_margins(int argc, char **argv) {
    (void)argc;
    (void)argv;
    rect_t wa;
    COVERAGE_ON();
    umka_sys_get_skin_margins(&wa);
    COVERAGE_OFF();
    printf("%" PRIu32 " left\n", wa.left);
    printf("%" PRIu32 " top\n", wa.top);
    printf("%" PRIu32 " right\n", wa.right);
    printf("%" PRIu32 " bottom\n", wa.bottom);
}

void shell_set_button_style(int argc, char **argv) {
    (void)argc;
    uint32_t style = strtoul(argv[1], NULL, 0);
    COVERAGE_ON();
    umka_sys_set_button_style(style);
    COVERAGE_OFF();
}

void shell_set_skin(int argc, char **argv) {
    (void)argc;
    const char *path = argv[1];
    COVERAGE_ON();
    int32_t status = umka_sys_set_skin(path);
    COVERAGE_OFF();
    printf("status: %" PRIi32 "\n", status);
}

void shell_get_font_smoothing(int argc, char **argv) {
    (void)argc;
    (void)argv;
    const char *names[] = {"off", "anti-aliasing", "subpixel"};
    COVERAGE_ON();
    int type = umka_sys_get_font_smoothing();
    COVERAGE_OFF();
    printf("font smoothing: %i - %s\n", type, names[type]);
}

void shell_set_font_smoothing(int argc, char **argv) {
    (void)argc;
    int type = strtol(argv[1], NULL, 0);
    COVERAGE_ON();
    umka_sys_set_font_smoothing(type);
    COVERAGE_OFF();
}

void shell_get_font_size(int argc, char **argv) {
    (void)argc;
    (void)argv;
    COVERAGE_ON();
    size_t size = umka_sys_get_font_size();
    COVERAGE_OFF();
    printf("%upx\n", size);
}

void shell_set_font_size(int argc, char **argv) {
    (void)argc;
    uint32_t size = strtoul(argv[1], NULL, 0);
    COVERAGE_ON();
    umka_sys_set_font_size(size);
    COVERAGE_OFF();
}

void shell_button(int argc, char **argv) {
    (void)argc;
    size_t x     = strtoul(argv[1], NULL, 0);
    size_t xsize = strtoul(argv[2], NULL, 0);
    size_t y     = strtoul(argv[3], NULL, 0);
    size_t ysize = strtoul(argv[4], NULL, 0);
    uint32_t button_id = strtoul(argv[5], NULL, 0);
    uint32_t color = strtoul(argv[6], NULL, 16);
    int draw_button = strtoul(argv[7], NULL, 0);
    int draw_frame = strtoul(argv[8], NULL, 0);
    COVERAGE_ON();
    umka_sys_button(x, xsize, y, ysize, button_id, draw_button, draw_frame, color);
    COVERAGE_OFF();
}

void shell_put_image(int argc, char **argv) {
    (void)argc;
    FILE *f = fopen(argv[1], "r");
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

void shell_put_image_palette(int argc, char **argv) {
    (void)argc;
    FILE *f = fopen(argv[1], "r");
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
    umka_sys_put_image_palette(image, xsize, ysize, x, y, bpp, palette, row_offset);
    COVERAGE_OFF();
    free(image);
}

void shell_draw_rect(int argc, char **argv) {
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

void shell_get_screen_size(int argc, char **argv) {
    (void)argc;
    (void)argv;
    uint32_t xsize, ysize;
    COVERAGE_ON();
    umka_sys_get_screen_size(&xsize, &ysize);
    COVERAGE_OFF();
    printf("%" PRIu32 "x%" PRIu32 "\n", xsize, ysize);
}

void shell_draw_line(int argc, char **argv) {
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

void shell_set_window_caption(int argc, char **argv) {
    (void)argc;
    const char *caption = argv[1];
    int encoding = strtoul(argv[2], NULL, 0);
    COVERAGE_ON();
    umka_sys_set_window_caption(caption, encoding);
    COVERAGE_OFF();
}


void shell_draw_window(int argc, char **argv) {
    (void)argc;
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
    umka_sys_draw_window(x, xsize, y, ysize, color, has_caption, client_relative, fill_workarea, gradient_fill, movable, style, caption);
    COVERAGE_OFF();
}

void shell_window_redraw(int argc, char **argv) {
    (void)argc;
    int begin_end = strtoul(argv[1], NULL, 0);
    COVERAGE_ON();
    umka_sys_window_redraw(begin_end);
    COVERAGE_OFF();
}

void shell_move_window(int argc, char **argv) {
    (void)argc;
    size_t x      = strtoul(argv[1], NULL, 0);
    size_t y      = strtoul(argv[2], NULL, 0);
    ssize_t xsize = strtol(argv[3], NULL, 0);
    ssize_t ysize = strtol(argv[4], NULL, 0);
    COVERAGE_ON();
    umka_sys_move_window(x, y, xsize, ysize);
    COVERAGE_OFF();
}

void shell_blit_bitmap(int argc, char **argv) {
    (void)argc;
    FILE *f = fopen(argv[1], "r");
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
    uint32_t params[] = {dstx, dsty, dstxsize, dstysize, srcx, srcy, srcxsize, srcysize, (uintptr_t)image, row_length};
    COVERAGE_ON();
    umka_sys_blit_bitmap(operation, background, transparent, client_relative, params);
    COVERAGE_OFF();
    free(image);
}

void shell_scrot(int argc, char **argv) {
    (void)argc;
    uint32_t xsize, ysize;
    COVERAGE_ON();
    umka_sys_get_screen_size(&xsize, &ysize);
    COVERAGE_OFF();

    uint32_t *lfb = kos_lfb_base;
    for (size_t y = 0; y < ysize; y++) {
        for (size_t x = 0; x < xsize; x++) {
            *lfb++ |= 0xff000000;
        }
    }

    unsigned error = lodepng_encode32_file(argv[1], (const unsigned char *)kos_lfb_base, xsize, ysize);
    if(error) printf("error %u: %s\n", error, lodepng_error_text(error));
}

void shell_cd(int argc, char **argv) {
    (void)argc;
    COVERAGE_ON();
    umka_sys_set_cwd(argv[1]);
    COVERAGE_OFF();
    cur_dir_changed = true;
}

void ls_range(f7080s1arg_t *fX0, f70or80_t f70or80) {
    f7080ret_t r;
    size_t bdfe_len = (fX0->encoding == CP866) ? BDFE_LEN_CP866 : BDFE_LEN_UNICODE;
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
        print_f70_status(&r, 1);
        f7080s1info_t *dir = fX0->buf;
        int ok = (r.count <= fX0->size);
        ok &= (dir->cnt == r.count);
        ok &= (r.status == ERROR_SUCCESS && r.count == fX0->size)
              || (r.status == ERROR_END_OF_FILE && r.count < fX0->size);
        assert(ok);
        if (!ok)
            break;
        bdfe_t *bdfe = dir->bdfes;
        for (size_t i = 0; i < dir->cnt; i++) {
            char fattr[KF_ATTR_CNT+1];
            convert_f70_file_attr(bdfe->attr, fattr);
            printf("%s %s\n", fattr, bdfe->name);
            bdfe = (bdfe_t*)((uintptr_t)bdfe + bdfe_len);
        }
        if (r.status == ERROR_END_OF_FILE) {
            break;
        }
    }
}

void ls_all(f7080s1arg_t *fX0, f70or80_t f70or80) {
    f7080ret_t r;
    size_t bdfe_len = (fX0->encoding == CP866) ? BDFE_LEN_CP866 : BDFE_LEN_UNICODE;
    while (true) {
        COVERAGE_ON();
        umka_sys_lfn(fX0, &r, f70or80);
        COVERAGE_OFF();
        print_f70_status(&r, 1);
        assert((r.status == ERROR_SUCCESS && r.count == fX0->size)
              || (r.status == ERROR_END_OF_FILE && r.count < fX0->size));
        f7080s1info_t *dir = fX0->buf;
        fX0->offset += dir->cnt;
        int ok = (r.count <= fX0->size);
        ok &= (dir->cnt == r.count);
        ok &= (r.status == ERROR_SUCCESS && r.count == fX0->size)
              || (r.status == ERROR_END_OF_FILE && r.count < fX0->size);
        assert(ok);
        if (!ok)
            break;
        printf("total = %"PRIi32"\n", dir->total_cnt);
        bdfe_t *bdfe = dir->bdfes;
        for (size_t i = 0; i < dir->cnt; i++) {
            char fattr[KF_ATTR_CNT+1];
            convert_f70_file_attr(bdfe->attr, fattr);
            printf("%s %s\n", fattr, bdfe->name);
            bdfe = (bdfe_t*)((uintptr_t)bdfe + bdfe_len);
        }
        if (r.status == ERROR_END_OF_FILE) {
            break;
        }
    }
}

fs_enc_t parse_encoding(const char *str) {
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

void shell_ls(int argc, char **argv, const char *usage, f70or80_t f70or80) {
    int opt;
    optind = 1;
    const char *optstring = (f70or80 == F70) ? "f:c:e:" : "f:c:e:p:";
    const char *path = ".";
    uint32_t readdir_enc = DEFAULT_READDIR_ENCODING;
    uint32_t path_enc = DEFAULT_PATH_ENCODING;
    uint32_t from_idx = 0, count = MAX_DIRENTS_TO_READ;
    if (argc > 1 && *argv[optind] != '-') {
        path = argv[optind++];
    }
    while ((opt = getopt(argc, argv, optstring)) != -1) {
        switch (opt) {
        case 'f':
            from_idx = strtoul(optarg, NULL, 0);
            break;
        case 'c':
            count = strtoul(optarg, NULL, 0);
            break;
        case 'e':
            readdir_enc = parse_encoding(optarg);
            break;
        case 'p':
            path_enc = parse_encoding(optarg);
            break;
        default:
            puts(usage);
            return;
        }
    }

    size_t bdfe_len = (readdir_enc <= CP866) ? BDFE_LEN_CP866 : BDFE_LEN_UNICODE;
    f7080s1info_t *dir = (f7080s1info_t*)malloc(sizeof(f7080s1info_t) + bdfe_len * MAX_DIRENTS_TO_READ);
    f7080s1arg_t fX0 = {.sf = 1, .offset = from_idx, .encoding = readdir_enc, .size = count, .buf = dir};
    if (f70or80 == F70) {
        fX0.u.f70.zero = 0;
        fX0.u.f70.path = path;
    } else {
        fX0.u.f80.path_encoding = path_enc;
        fX0.u.f80.path = path;
    }
    if (count != MAX_DIRENTS_TO_READ) {
        ls_range(&fX0, f70or80);
    } else {
        ls_all(&fX0, f70or80);
    }
    free(dir);
    return;
}

void shell_ls70(int argc, char **argv) {
    const char *usage = \
        "usage: ls70 [dir] [option]...\n"
        "  -f number        index of the first dir entry to read\n"
        "  -c number        number of dir entries to read\n"
        "  -e encoding      cp866|utf16|utf8\n"
        "                   return directory listing in this encoding";
    shell_ls(argc, argv, usage, F70);
}

void shell_ls80(int argc, char **argv) {
    const char *usage = \
        "usage: ls80 [dir] [option]...\n"
        "  -f number        index of the first dir entry to read\n"
        "  -c number        number of dir entries to read\n"
        "  -e encoding      cp866|utf16|utf8\n"
        "                   return directory listing in this encoding\n"
        "  -p encoding      cp866|utf16|utf8\n"
        "                   path to dir is specified in this encoding";
    shell_ls(argc, argv, usage, F80);
}

void shell_stat(int argc, char **argv, f70or80_t f70or80) {
    (void)argc;
    f7080s5arg_t fX0 = {.sf = 5, .flags = 0};
    f7080ret_t r;
    bdfe_t file;
    fX0.buf = &file;
    if (f70or80 == F70) {
        fX0.u.f70.zero = 0;
        fX0.u.f70.path = argv[1];
    } else {
        fX0.u.f80.path_encoding = DEFAULT_PATH_ENCODING;
        fX0.u.f80.path = argv[1];
    }
    COVERAGE_ON();
    umka_sys_lfn(&fX0, &r, f70or80);
    COVERAGE_OFF();
    print_f70_status(&r, 0);
    if (r.status != ERROR_SUCCESS)
        return;
    char fattr[KF_ATTR_CNT+1];
    convert_f70_file_attr(file.attr, fattr);
    printf("attr: %s\n", fattr);
    if ((file.attr & KF_FOLDER) == 0) {   // don't show size for dirs
        printf("size: %llu\n", file.size);
    }

#if PRINT_DATE_TIME == 1
    time_t time;
    struct tm *t;
    time = kos_time_to_epoch(&file.ctime);
    t = localtime(&time);
    printf("ctime: %4.4i.%2.2i.%2.2i %2.2i:%2.2i:%2.2i\n",
           t->tm_year + 1900, t->tm_mon + 1, t->tm_mday,
           t->tm_hour, t->tm_min, t->tm_sec);
    time = kos_time_to_epoch(&file.atime);
    t = localtime(&time);
    printf("atime: %4.4i.%2.2i.%2.2i %2.2i:%2.2i:%2.2i\n",
           t->tm_year + 1900, t->tm_mon + 1, t->tm_mday,
           t->tm_hour, t->tm_min, t->tm_sec);
    time = kos_time_to_epoch(&file.mtime);
    t = localtime(&time);
    printf("mtime: %4.4i.%2.2i.%2.2i %2.2i:%2.2i:%2.2i\n",
           t->tm_year + 1900, t->tm_mon + 1, t->tm_mday,
           t->tm_hour, t->tm_min, t->tm_sec);
#endif
    return;
}

void shell_stat70(int argc, char **argv) {
    shell_stat(argc, argv, F70);
}

void shell_stat80(int argc, char **argv) {
    shell_stat(argc, argv, F80);
}

void shell_read(int argc, char **argv, f70or80_t f70or80) {
    (void)argc;
    f7080s0arg_t fX0 = {.sf = 0};
    f7080ret_t r;
    bool dump_bytes = false, dump_hash = false;
    if (argc < 4) {
        printf("usage: %s <offset> <length> [-b] [-h] [-e cp866|utf8|utf16]\n", argv[0]);
        return;
    }
    int opt = 1;
    if (f70or80 == F70) {
        fX0.u.f70.zero = 0;
        fX0.u.f70.path = argv[opt++];
    } else {
        fX0.u.f80.path_encoding = DEFAULT_PATH_ENCODING;
        fX0.u.f80.path = argv[opt++];
    }
    if ((opt >= argc) || !parse_uint64(argv[opt++], &fX0.offset))
        return;
    if ((opt >= argc) || !parse_uint32(argv[opt++], &fX0.count))
        return;
    for (; opt < argc; opt++) {
        if (!strcmp(argv[opt], "-b")) {
            dump_bytes = true;
        } else if (!strcmp(argv[opt], "-h")) {
            dump_hash = true;
        } else if (!strcmp(argv[opt], "-e")) {
            if (f70or80 == F70) {
                printf("f70 doesn't accept encoding parameter, use f80\n");
                return;
            }
        } else {
            printf("invalid option: '%s'\n", argv[opt]);
            return;
        }
    }
    fX0.buf = (uint8_t*)malloc(fX0.count);

    COVERAGE_ON();
    umka_sys_lfn(&fX0, &r, f70or80);
    COVERAGE_OFF();

    print_f70_status(&r, 1);
    if (r.status == ERROR_SUCCESS || r.status == ERROR_END_OF_FILE) {
        if (dump_bytes)
            print_bytes(fX0.buf, r.count);
        if (dump_hash)
            print_hash(fX0.buf, r.count);
    }

    free(fX0.buf);
    return;
}

void shell_read70(int argc, char **argv) {
    shell_read(argc, argv, F70);
}

void shell_read80(int argc, char **argv) {
    shell_read(argc, argv, F80);
}

typedef struct {
    char *name;
    void (*func) (int, char **);
} func_table_t;

func_table_t funcs[] = {
                        { "disk_add",           shell_disk_add },
                        { "disk_del",           shell_disk_del },
                        { "ls70",               shell_ls70 },
                        { "ls80",               shell_ls80 },
                        { "stat70",             shell_stat70 },
                        { "stat80",             shell_stat80 },
                        { "read70",             shell_read70 },
                        { "read80",             shell_read80 },
                        { "pwd",                shell_pwd },
                        { "cd",                 shell_cd },
                        { "set_cwd",            shell_cd },
                        { "draw_window",        shell_draw_window },
                        { "set_pixel",          shell_set_pixel },
                        { "write_text",         shell_write_text },
                        { "put_image",          shell_put_image },
                        { "button",             shell_button },
                        { "process_info",       shell_process_info },
                        { "window_redraw",      shell_window_redraw },
                        { "draw_rect",          shell_draw_rect },
                        { "get_screen_size",    shell_get_screen_size },
                        { "draw_line",          shell_draw_line },
                        { "display_number",     shell_display_number },
                        { "set_button_style",   shell_set_button_style },
                        { "set_window_colors",  shell_set_window_colors },
                        { "get_window_colors",  shell_get_window_colors },
                        { "get_skin_height",    shell_get_skin_height },
                        { "get_screen_area",    shell_get_screen_area },
                        { "set_screen_area",    shell_set_screen_area },
                        { "get_skin_margins",   shell_get_skin_margins },
                        { "set_skin",           shell_set_skin },
                        { "get_font_smoothing", shell_get_font_smoothing },
                        { "set_font_smoothing", shell_set_font_smoothing },
                        { "get_font_size",      shell_get_font_size },
                        { "set_font_size",      shell_set_font_size },
                        { "put_image_palette",  shell_put_image_palette },
                        { "move_window",        shell_move_window },
                        { "set_window_caption", shell_set_window_caption },
                        { "blit_bitmap",        shell_blit_bitmap },
                        { "scrot",              shell_scrot },
                        { "dump_win_stack",     shell_dump_win_stack },
                        { "dump_win_pos",       shell_dump_win_pos },
                        { NULL,                 NULL },
                       };

void *run_test(const char *infile_name) {
    FILE *infile, *outfile;

    if (!infile_name) {
        infile = stdin;
        outfile = stdout;
    } else {
        char outfile_name[PATH_MAX];
        strncpy(outfile_name, infile_name, PATH_MAX-2);    // ".t" is shorter than ".out"
        char *last_dot = strrchr(outfile_name, '.');
        if (!last_dot) {
            printf("test file must have '.t' suffix\n");
            return NULL;
        }
        strcpy(last_dot, ".out.log");
        infile = fopen(infile_name, "r");
        outfile = fopen(outfile_name, "w");
        if (!infile || !outfile) {
            printf("can't open in/out files\n");
            return NULL;
        }
    }

    int is_tty = isatty(fileno(infile));
    char **argv = (char**)malloc(sizeof(char*) * (MAX_COMMAND_ARGS + 1));
    while(next_line(infile, is_tty)) {
        if (cmd_buf[0] == '#' || cmd_buf[0] == '\n') {
            printf("%s", cmd_buf);
            continue;
        }
        if (cmd_buf[0] == 'X') break;
        if (!is_tty) {
            prompt();
            printf("%s", cmd_buf);
            fflush(outfile);
        }
        int argc = split_args(cmd_buf, argv);
        func_table_t *ft;
        for (ft = funcs; ft->name != NULL; ft++) {
            if (!strcmp(argv[0], ft->name)) {
                break;
            }
        }
        if (ft->name) {
            ft->func(argc, argv);
        } else {
            printf("unknown command: %s\n", argv[0]);
        }
    }
    free(argv);

    return NULL;
}

int main(int argc, char **argv) {
    const char *usage = \
        "usage: umka_shell [test_file.t] [-c]\n"
        "  -c               collect coverage";
    const char *test_file = NULL;

    int opt;
    optind = 1;
    if (argc > 1 && *argv[optind] != '-') {
        test_file = argv[optind++];
    }
    while ((opt = getopt(argc, argv, "c")) != -1) {
        switch (opt) {
        case 'c':
            coverage = 1;
            break;
        default:
            puts(usage);
            return 1;
        }
    }

    if (coverage)
        trace_begin();

    COVERAGE_ON();
    kos_init();
    COVERAGE_OFF();

    run_test(test_file);

    if (coverage)
        trace_end();

    return 0;
}

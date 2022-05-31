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

#include <inttypes.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "optparse.h"
#include "shell.h"
#include "umka.h"
#include "trace.h"

#define HIST_FILE_BASENAME ".umka_shell.history"
#define UMKA_DEFAULT_DISPLAY_WIDTH 400
#define UMKA_DEFAULT_DISPLAY_HEIGHT 300

char history_filename[PATH_MAX];

void build_history_filename() {
    const char *dir_name;
    if (!(dir_name = getenv("HOME"))) {
        dir_name = ".";
    }
    sprintf(history_filename, "%s/%s", dir_name, HIST_FILE_BASENAME);
}

/*
uint8_t mem0[64*1024*1024];
uint8_t mem1[128*1024*1024];
uint8_t mem2[256*1024*1024];
*/

int
main(int argc, char **argv) {
    (void)argc;
    umka_tool = UMKA_SHELL;
    const char *usage = \
        "usage: umka_shell [-i infile] [-o outfile] [-r] [-c] [-h]\n"
        "  -i infile        file with commands\n"
        "  -o outfile       file for logs\n"
        "  -r               reproducible logs (without pointers and datetime)\n"
        "  -c               collect coverage";
    const char *infile = NULL, *outfile = NULL;
    struct shell_ctx ctx = {.reproducible = 0, .hist_file = history_filename,
                            .var = NULL};
    build_history_filename();
/*
    kos_boot.memmap_block_cnt = 3;
    kos_boot.memmap_blocks[0] = (e820entry_t){(uintptr_t)mem0, 64*1024*1024, 1};
    kos_boot.memmap_blocks[1] = (e820entry_t){(uintptr_t)mem1, 128*1024*1024, 1};
    kos_boot.memmap_blocks[2] = (e820entry_t){(uintptr_t)mem2, 256*1024*1024, 1};
*/
    kos_boot.bpp = 32;
    kos_boot.x_res = UMKA_DEFAULT_DISPLAY_WIDTH;
    kos_boot.y_res = UMKA_DEFAULT_DISPLAY_HEIGHT;
    kos_boot.pitch = UMKA_DEFAULT_DISPLAY_WIDTH*4;  // 32bpp

    struct optparse options;
    int opt;
    optparse_init(&options, argv);

    while ((opt = optparse(&options, "i:o:rc")) != -1) {
        switch (opt) {
        case 'i':
            infile = options.optarg;
            break;
        case 'o':
            outfile = options.optarg;
            break;
        case 'r':
            ctx.reproducible = 1;
            break;
        case 'c':
            coverage = 1;
            break;
        case 'h':
            fputs(usage, stderr);
            exit(0);
        default:
            fprintf(stderr, "bad option: %c\n", opt);
            fputs(usage, stderr);
            exit(1);
        }
    }
    if (infile && !freopen(infile, "r", stdin)) {
        fprintf(stderr, "[!] can't open file for reading: %s", infile);
        exit(1);
    }
    if (outfile && !freopen(outfile, "w", stdout)) {
        fprintf(stderr, "[!] can't open file for writing: %s", outfile);
        exit(1);
    }

    if (coverage)
        trace_begin();

    run_test(&ctx);

    if (coverage)
        trace_end();

    return 0;
}

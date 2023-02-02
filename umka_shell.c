/*
    SPDX-License-Identifier: GPL-2.0-or-later

    UMKa - User-Mode KolibriOS developer tools
    umka_shell - the shell

    Copyright (C) 2017-2023  Ivan Baravy <dunkaist@gmail.com>
    Copyright (C) 2021  Magomed Kostoev <mkostoevr@yandex.ru>
*/

#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "optparse.h"
#include "shell.h"
#include "umkaio.h"
#include "trace.h"

#define HIST_FILE_BASENAME ".umka_shell.history"

struct umka_shell_ctx {
    struct umka_ctx *umka;
    struct umka_io *io;
    struct shell_ctx *shell;
};

char history_filename[PATH_MAX];

struct umka_shell_ctx *
umka_shell_init(int reproducible, FILE *fin, FILE *fout) {
    struct umka_shell_ctx *ctx = malloc(sizeof(struct umka_shell_ctx));
    ctx->umka = umka_init();
    ctx->io = io_init(NULL);
    ctx->shell = shell_init(reproducible, history_filename, ctx->umka, ctx->io,
                            fin, fout, &ctx->umka->running);
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
uint8_t mem0[64*1024*1024];
uint8_t mem1[128*1024*1024];
uint8_t mem2[256*1024*1024];
*/

int
main(int argc, char **argv) {
    (void)argc;
    const char *usage = \
        "usage: umka_shell [-i infile] [-o outfile] [-r] [-c] [-h]\n"
        "  -i infile        file with commands\n"
        "  -o outfile       file for logs\n"
        "  -r               reproducible logs (without pointers and datetime)\n"
        "  -c               collect coverage\n";

    const char *infile = NULL, *outfile = NULL;
    FILE *fin = stdin;
    FILE *fout = stdout;
    build_history_filename();
/*
    kos_boot.memmap_block_cnt = 3;
    kos_boot.memmap_blocks[0] = (e820entry_t){(uintptr_t)mem0, 64*1024*1024, 1};
    kos_boot.memmap_blocks[1] = (e820entry_t){(uintptr_t)mem1, 128*1024*1024, 1};
    kos_boot.memmap_blocks[2] = (e820entry_t){(uintptr_t)mem2, 256*1024*1024, 1};
*/
    kos_boot.bpp = UMKA_DEFAULT_DISPLAY_BPP;
    kos_boot.x_res = UMKA_DEFAULT_DISPLAY_WIDTH;
    kos_boot.y_res = UMKA_DEFAULT_DISPLAY_HEIGHT;
    kos_boot.pitch = UMKA_DEFAULT_DISPLAY_WIDTH * UMKA_DEFAULT_DISPLAY_BPP / 8;

    int reproducible = 0;

    struct optparse options;
    optparse_init(&options, argv);
    int opt;

    while ((opt = optparse(&options, "i:o:rc")) != -1) {
        switch (opt) {
        case 'i':
            infile = options.optarg;
            break;
        case 'o':
            outfile = options.optarg;
            break;
        case 'r':
            reproducible = 1;
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

    if (infile) {
        fin = fopen(infile, "r");
        if (!fin) {
            fprintf(stderr, "[!] can't open file for reading: %s\n", infile);
            exit(1);
        }
    }
    if (outfile) {
        fout = fopen(outfile, "w");
        if (!fout) {
            fprintf(stderr, "[!] can't open file for writing: %s\n", outfile);
            exit(1);
        }
    }

    struct umka_shell_ctx *ctx = umka_shell_init(reproducible, fin, fout);

    if (coverage)
        trace_begin();

    run_test(ctx->shell);

    if (coverage) {
        trace_end();
        char coverage_filename[32];
        sprintf(coverage_filename, "coverage.%i", getpid());
        FILE *f = fopen(coverage_filename, "w");
        fwrite(coverage_table,
               COVERAGE_TABLE_SIZE * sizeof(struct coverage_branch), 1, f);
        fclose(f);
    }

    return 0;
}

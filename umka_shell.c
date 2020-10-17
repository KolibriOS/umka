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

#include <fcntl.h>
#include <inttypes.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include "shell.h"
#include "umka.h"
#include "trace.h"

int
main(int argc, char **argv) {
    umka_tool = UMKA_SHELL;
    const char *usage = \
        "usage: umka_shell [test_file.t] [-c]\n"
        "  -c               collect coverage";
    const char *infile = NULL;
    char outfile[PATH_MAX] = {0};
    FILE *fin = stdin, *fout = stdout;

    int opt;
    optind = 1;
    if (argc > 1 && *argv[optind] != '-') {
        infile = argv[optind++];
        strncpy(outfile, infile, PATH_MAX-2);    // ".t" is shorter than ".out"
        char *last_dot = strrchr(outfile, '.');
        if (!last_dot) {
            printf("test file must have '.t' suffix\n");
            return 1;
        }
        strcpy(last_dot, ".out.log");
    }

    if (infile) {
        fin = fopen(infile, "r");
    }
    if (*outfile) {
        fout = fopen(outfile, "w");
    }
    if (!fin || !fout) {
        printf("can't open in/out files\n");
        return 1;
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
    umka_init();
    COVERAGE_OFF();

    run_test(fin, fout, 1);

    if (coverage)
        trace_end();

    return 0;
}

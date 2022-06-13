/*
    SPDX-License-Identifier: GPL-2.0-or-later

    UMKa - User-Mode KolibriOS developer tools

    Copyright (C) 2020,2022  Ivan Baravy <dunkaist@gmail.com>
*/

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define COVERAGE_TABLE_SIZE (512*1024)

struct coverage_branch {
    uint64_t to_cnt;
    uint64_t from_cnt;
};

struct coverage_branch branches[COVERAGE_TABLE_SIZE];

uint32_t coverage_begin = 0x34; // TODD: detect in runtime

void read_coverage_file(const char *fname) {
    FILE *f = fopen(fname, "r");
    for (size_t i = 0; i < COVERAGE_TABLE_SIZE; i++) {
        uint64_t from, to;
        fread(&to, 1, sizeof(uint64_t), f);
        fread(&from, 1, sizeof(uint64_t), f);
        branches[i].to_cnt += to;
        branches[i].from_cnt += from;
    }
    fclose(f);
}

size_t count_line_bytes(const char *s) {
    size_t cnt = 0;
    for (size_t i = 10; i <= 58; i += 3) {
        if (s[i] == ' ') break;
        cnt++;
    }
    return cnt;
}

size_t count_block_bytes(FILE *f) {
    char tmp[1024];
    size_t cnt = 0;
    if (fgets(tmp, 1024, f) && strspn(tmp, "0123456789ABCDEF") == 8) {
        cnt = count_line_bytes(tmp);
        while (fgets(tmp, 1024, f) && tmp[0] == ' ' && tmp[10] != ' ') {
            cnt += count_line_bytes(tmp);
        }
    }
    return cnt;
}

int is_cond_jump(const char *s) {
    s += strspn(s, " \t");
    int found = !strncmp(s, "jo", 2)
                || !strncmp(s, "jno", 3)
                || !strncmp(s, "js", 2)
                || !strncmp(s, "jns", 3)
                || !strncmp(s, "je", 2)
                || !strncmp(s, "jne", 3)
                || !strncmp(s, "jz", 2)
                || !strncmp(s, "jnz", 3)
                || !strncmp(s, "jb", 2)
                || !strncmp(s, "jnb", 3)
                || !strncmp(s, "jc", 2)
                || !strncmp(s, "jnc", 3)
                || !strncmp(s, "jae", 3)
                || !strncmp(s, "jnae", 4)
                || !strncmp(s, "jbe", 3)
                || !strncmp(s, "jna", 3)
                || !strncmp(s, "ja", 2)
                || !strncmp(s, "jnbe", 4)
                || !strncmp(s, "jl", 2)
                || !strncmp(s, "jnge", 4)
                || !strncmp(s, "jge", 3)
                || !strncmp(s, "jnl", 3)
                || !strncmp(s, "jle", 3)
                || !strncmp(s, "jng", 3)
                || !strncmp(s, "jg",2)
                || !strncmp(s, "jnle", 4)
                || !strncmp(s, "jp", 2)
                || !strncmp(s, "jpe", 3)
                || !strncmp(s, "jnp", 3)
                || !strncmp(s, "jpo", 3)
                || !strncmp(s, "loop", 4)
                || !strncmp(s, "jcxz", 4)
                || !strncmp(s, "jecxz", 5);
    return found;
}

int main(int argc, char **argv) {
    if (argc < 3) {
        fprintf(stderr, "usage: covpreproc <listing file> <coverage files ...>\n");
        exit(1);
    }

    for (int i = 2; i < argc; i++) {
        read_coverage_file(argv[i]);
    }

    FILE *f = fopen(argv[1], "r");
    char tmp[1024];
    uint64_t cur = 0, taken, not_taken;
    while (1) {
        off_t fpos_before = ftello(f);
        if (!fgets(tmp, 1024, f)) {
            break;
        }
        off_t fpos_after = ftello(f);
        if (strspn(tmp, "0123456789ABCDEF") == 8) {
            int is_cond = is_cond_jump(tmp + 64);
            fseeko(f, fpos_before, SEEK_SET);
            size_t inst_len = count_block_bytes(f);
            fseeko(f, fpos_after, SEEK_SET);
            unsigned long pos = strtoul(tmp, NULL, 16);
            size_t total_to = 0, total_from = 0;
            for (size_t i = 0; i < inst_len; i++) {
                if (pos + i >= coverage_begin
                    && pos + i < coverage_begin + COVERAGE_TABLE_SIZE) {
                    total_to += branches[pos + i - coverage_begin].to_cnt;
                    total_from += branches[pos + i - coverage_begin].from_cnt;
                }
            }
            cur += total_to;
            if (is_cond) {
                taken = total_from;
                not_taken = cur - taken;
                if (taken && not_taken) {
                    putchar(' ');
                } else {
                    putchar('-');
                }
            } else {
                putchar(' ');
            }
            if (cur) {
                putchar(' ');
            } else {
                putchar('-');
            }
            if (is_cond) {
                int spaces = 19 - printf("%10" PRIu64 "/%" PRIu64, taken,
                                         not_taken);
                while (spaces-- > 0)
                    putchar(' ');
            } else {
                printf("                   ");
            }
            printf(" %10" PRIu64, cur);
            cur -= total_from;
        } else {
            printf("                                ");
        }
        printf(" : %s", tmp + 64);
    }
    fclose(f);

    return 0;
}

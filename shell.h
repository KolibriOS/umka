#ifndef SHELL_H_INCLUDED
#define SHELL_H_INCLUDED

#include <stdio.h>

struct shell_ctx {
    FILE *fin;
    FILE *fout;
    int reproducible;
};

void *run_test(struct shell_ctx *ctx);

#endif  // SHELL_H_INCLUDED

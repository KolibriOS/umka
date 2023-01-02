/*
    SPDX-License-Identifier: GPL-2.0-or-later

    UMKa - User-Mode KolibriOS developer tools
    mkdoubledirs - make directories with same names inside: dirname/dirname,
    useful for search testing

    Copyright (C) 2022  Ivan Baravy <dunkaist@gmail.com>
*/

#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>

int main(int argc, char *argv[])
{
    const char *prefix;
    const char *path;
    unsigned count;
    if (argc != 4) {
        fprintf(stderr, "mkdoubledirs <directory> <prefix> <count>\n");
        exit(1);
    }
    path = argv[1];
    prefix = argv[2];
    sscanf(argv[3], "%u", &count);

    if (chdir(path)) {
        fprintf(stderr, "Can't change dir to %s: %s\n", path, strerror(errno));
        exit(1);
    }
    char dirname[256];

    for(unsigned cur = 0; cur < count; cur++) {
        sprintf(dirname, "%s%10.10u", prefix, cur);
        if(mkdirat(AT_FDCWD, dirname, 0755)) {
            fprintf(stderr, "Can't mkdir %s: %s\n", dirname, strerror(errno));
            exit(1);
        }
        int dirfd = open(dirname, O_DIRECTORY);
        if (dirfd == -1) {
            fprintf(stderr, "Can't open %s: %s\n", dirname, strerror(errno));
            exit(1);
        }
        if(mkdirat(dirfd, dirname, 0755)) {
            fprintf(stderr, "Can't mkdir %s: %s\n", dirname, strerror(errno));
            exit(1);
        }
        close(dirfd);
    }

    return 0;
}

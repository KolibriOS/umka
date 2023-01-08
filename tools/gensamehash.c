/*
    SPDX-License-Identifier: GPL-2.0-or-later

    UMKa - User-Mode KolibriOS developer tools
    gensamehash - make directories with same names inside: dirname/dirname,
    useful for search testing

    Copyright (C) 2022  Ivan Baravy <dunkaist@gmail.com>
*/

#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <signal.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#define NUM_LEN 10
#define rol32(x,y)  (((x) << (y)) | ((x) >> (32 - (y))))

// the hash function has been taken from XFS docs with minor edits
uint32_t xfs_da_hashname(const char *name, int namelen) {
    uint32_t hash;

    /*
     * Do four characters at a time as long as we can.
     */
    for (hash = 0; namelen >= 4; namelen -= 4, name += 4) {
        hash = (name[0] << 21) ^ (name[1] << 14) ^ (name[2] << 7) ^
               (name[3] << 0) ^ rol32(hash, 7 * 4);
    }
    /*
     * Now do the rest of the characters.
     */
    switch (namelen) {
    case 3:
        return (name[0] << 14) ^ (name[1] << 7) ^ (name[2] << 0) ^
               rol32(hash, 7 * 3);
    case 2:
        return (name[0] << 7) ^ (name[1] << 0) ^ rol32(hash, 7 * 2);
    case 1:
        return (name[0] << 0) ^ rol32(hash, 7 * 1);
    default: /* case 0: */
        return hash;
    }
}

char name[256];
char hash_filename[256];

void increment(char *num) {
    for (char *d = num + NUM_LEN - 1; d >= num; d--) {
        switch (d[0]) {
        case '9':
            d[0] = 'A';
            return;
        case 'Z':
            d[0] = 'a';
            return;
        case 'z':
            d[0] = '0';
            break;
        default:
            d[0]++;
            return;
        }
    }
}

void int_handler(int signo, siginfo_t *info, void *context) {
    (void)signo;
    (void)info;
    (void)context;
    fprintf(stderr, "# cur name is %s\n", name);
}

int main(int argc, char *argv[])
{
    uint32_t hash;
    int hash_cnt = 0;
    uint64_t start_num;
    if (argc != 4) {
        fprintf(stderr, "gensamehash <hash> <hash_cnt> <start_num>\n");
        exit(1);
    }

    hash = strtoul(argv[1], NULL, 16);
    sscanf(argv[2], "%i", &hash_cnt);
    sscanf(argv[3], "%" SCNu64, &start_num);

    fprintf(stderr, "pid: %u\n", getpid());
    sprintf(hash_filename, "hash_0x%8.8" PRIx32 ".%u", hash, getpid());

    sprintf(name, "%10.10" PRIu64, start_num);

    struct sigaction sa;
    sa.sa_sigaction = int_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_SIGINFO;

    if (sigaction(SIGUSR1, &sa, NULL) == -1) {
        fprintf(stderr, "Can't install SIGUSR1 handler!\n");
        exit(1);
    }

    while (true) {
        uint32_t h = xfs_da_hashname(name, NUM_LEN);
        if (h == hash) {
            FILE *f = fopen(hash_filename, "a+");
            fprintf(f, "%7.7u %s\n", hash_cnt, name);
            fclose(f);
            hash_cnt++;
        }
//        fprintf(stderr, "# hash of %s is 0x%8.8x\n", name, h);
        increment(name);
    }

    return 0;
}

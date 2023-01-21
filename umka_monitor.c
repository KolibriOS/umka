/*
    SPDX-License-Identifier: GPL-2.0-or-later

    UMKa - User-Mode KolibriOS developer tools
    umka_monitor - a program to monitor and control umka_os

    Copyright (C) 2023  Ivan Baravy <dunkaist@gmail.com>
*/

#include <fcntl.h>
#include <inttypes.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <threads.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/uio.h>
#include <unistd.h>
#include <SDL2/SDL.h>
#include "umka_os.h"
#include "bestline.h"
#include "optparse.h"

struct shared_info sinfo;

int
sdlthr(void *arg) {
    (void)arg;
    if(SDL_Init(SDL_INIT_VIDEO) < 0)
    {
        fprintf(stderr, "Failed to initialize the SDL2 library\n");
        return -1;
    }

    SDL_Window *window = SDL_CreateWindow("LFB Viewer SDL2",
                                          SDL_WINDOWPOS_CENTERED,
                                          SDL_WINDOWPOS_CENTERED,
                                          sinfo.lfb_width, sinfo.lfb_height,
                                          0);

    if(!window)
    {
        fprintf(stderr, "Failed to create window\n");
        return -1;
    }

    SDL_Surface *window_surface = SDL_GetWindowSurface(window);

    if(!window_surface)
    {
        fprintf(stderr, "Failed to get the surface from the window\n");
        return -1;
    }

    uint32_t *lfb = (uint32_t*)malloc(sinfo.lfb_width*sinfo.lfb_height*sizeof(uint32_t));

    struct iovec remote = {.iov_base = (void*)(uintptr_t)sinfo.lfb_base,
                           .iov_len = sinfo.lfb_width*sinfo.lfb_height*4};
    struct iovec local = {.iov_base = lfb,
                          .iov_len = sinfo.lfb_width*sinfo.lfb_height*4};


    uint32_t *p = window_surface->pixels;
    while (1) {
        process_vm_readv(sinfo.pid, &local, 1, &remote, 1, 0);
        memcpy(window_surface->pixels, lfb, 400*300*4);
        SDL_LockSurface(window_surface);
        for (ssize_t y = 0; y < window_surface->h; y++) {
            for (ssize_t x = 0; x < window_surface->w; x++) {
                p[y*window_surface->pitch/4+x] = lfb[y*400+x];
            }
        }
        SDL_UnlockSurface(window_surface);
        SDL_UpdateWindowSurface(window);
        sleep(1);
    }


    return 0;
}

int
main(int argc, char *argv[]) {
    (void)argc;
    const char *usage = "umka_monitor [-i <infile>] [-o <outfile>] [-s <shname>]\n";
    const char *shname = "/umka";
    int shfd = 0;
    const char *infile = NULL, *outfile = NULL;

    struct optparse options;
    int opt;
    optparse_init(&options, argv);

    while ((opt = optparse(&options, "i:o:s:")) != -1) {
        switch (opt) {
        case 'i':
            infile = options.optarg;
            break;
        case 'o':
            outfile = options.optarg;
            break;
        case 's':
            shname = options.optarg;
            break;
        default:
            fprintf(stderr, "bad option: %c\n", opt);
            fputs(usage, stderr);
            exit(1);
        }
    }

    if (infile && !freopen(infile, "r", stdin)) {
        fprintf(stderr, "[!] can't open file for reading: %s\n", infile);
        exit(1);
    }
    if (outfile && !freopen(outfile, "w", stdout)) {
        fprintf(stderr, "[!] can't open file for writing: %s\n", outfile);
        exit(1);
    }

    if (shname) {
        shfd = shm_open(shname, O_RDONLY, S_IRUSR | S_IWUSR);
        if (!shfd) {
            perror("[!] can't open shared memory");
            exit(1);
        }
    }
    ssize_t nread = read(shfd, &sinfo, sizeof(sinfo));
    if (nread != sizeof(sinfo)) {
        perror("can't read from shared memory");
        exit(1);
    }
    printf("read umka_os configuration:\n"
           "  pid: %d\n"
           "  lfb_base: %p\n"
           "  lfb_bpp: %u"
           "  lfb_width: %u"
           "  lfb_height: %u"
           "  cmd: %p\n",
           (pid_t)sinfo.pid, (void*)(uintptr_t)sinfo.lfb_base, sinfo.lfb_bpp,
           sinfo.lfb_width, sinfo.lfb_height, (void*)(uintptr_t)sinfo.cmd_buf);
    shm_unlink(shname);

    union sigval sval = (union sigval){.sival_int = 14};

    thrd_t st;
    thrd_create(&st, sdlthr, NULL);

    while (1) {
        getchar();
        sigqueue(sinfo.pid, SIGUSR2, sval);
    }

    return 0;
}

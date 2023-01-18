/*
    SPDX-License-Identifier: GPL-2.0-or-later

    UMKa - User-Mode KolibriOS developer tools
    lfbview - KolibriOS framebuffer viewer via SDL2

    Copyright (C) 2023  Ivan Baravy <dunkaist@gmail.com>
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/uio.h>
#include <unistd.h>
#include <SDL2/SDL.h>

int
main(int argc, char *argv[]) {
    const char *usage = "usage: lfbview <pid> <address> <width> <height>";
    if (argc != 5) {
        puts(usage);
        exit(1);
    }
    int depth = 32;
    int umka_pid = strtol(argv[1], NULL, 0);
    uintptr_t umka_lfb_addr = strtol(argv[2], NULL, 0);
    size_t lfb_width = strtoul(argv[3], NULL, 0);
    size_t lfb_height = strtoul(argv[4], NULL, 0);

    if(SDL_Init(SDL_INIT_VIDEO) < 0)
    {
        fprintf(stderr, "Failed to initialize the SDL2 library\n");
        return -1;
    }

    SDL_Window *window = SDL_CreateWindow("LFB Viewer SDL2",
                                          SDL_WINDOWPOS_CENTERED,
                                          SDL_WINDOWPOS_CENTERED,
                                          400, 300,
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

    uint32_t *lfb = (uint32_t*)malloc(lfb_width*lfb_height*sizeof(uint32_t));

    struct iovec remote = {.iov_base = (void*)umka_lfb_addr,
                           .iov_len = lfb_width*lfb_height*4};
    struct iovec local = {.iov_base = lfb,
                          .iov_len = lfb_width*lfb_height*4};


    uint32_t *p = window_surface->pixels;
    while (1) {
        process_vm_readv(umka_pid, &local, 1, &remote, 1, 0);
        memcpy(window_surface->pixels, lfb, 400*300*4);
        SDL_LockSurface(window_surface);
        for (size_t y = 0; y < window_surface->h; y++) {
            for (size_t x = 0; x < window_surface->w; x++) {
                p[y*window_surface->pitch/4+x] = lfb[y*400+x];
            }
        }
        SDL_UnlockSurface(window_surface);
        SDL_UpdateWindowSurface(window);
        sleep(1);
    }

    return 0;
}

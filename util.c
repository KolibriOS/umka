/*
    SPDX-License-Identifier: GPL-2.0-or-later

    UMKa - User-Mode KolibriOS developer tools

    Copyright (C) 2021, 2023  Ivan Baravy <dunkaist@gmail.com>
*/

#include <stdio.h>
#include "umka.h"

struct devices_dat_entry {
    uint8_t fun:3;
    uint8_t dev:5;
    uint8_t bus;
    uint16_t pad1;
    uint16_t vendor_id;
    uint16_t device_id;
    uint32_t irq;
    uint32_t pad2;
};

STDCALL void*
dump_devices_dat_iter(struct pci_dev *node, void *arg) {
    FILE *f = arg;
    if (node->gsi) {
        fwrite(&(struct devices_dat_entry){.dev = node->dev,
                                           .fun = node->fun,
                                           .bus = node->parent->bus,
                                           .vendor_id = node->vendor_id,
                                           .device_id = node->device_id,
                                           .irq = node->gsi},
              1, sizeof(struct devices_dat_entry), f);
    }
    return NULL;
}

void
dump_devices_dat(const char *filename) {
    FILE *f = fopen(filename, "w");
    if (!f) {
        perror("Can't open file devices.dat");
        return;
    }
    kos_pci_walk_tree(kos_pci_root, NULL, dump_devices_dat_iter, f);
//    dump_devices_dat_iter(kos_pci_root, f);
    fwrite(&(uint32_t){0xffffffffu}, 1, 4, f);
    fclose(f);
}

static void
copy_display_bpp16_to_rgb888(void *to) {
    for (size_t y = 0; y < kos_display.height; y++) {
        for (size_t x = 0; x < kos_display.width; x++) {
            uint32_t p = ((uint16_t*)kos_lfb_base)[y*kos_display.width+x];
            p = ((p & 0xf800u) << 8) + ((p & 0x7e0u) << 5) + ((p & 0x1fu) << 3);
            ((uint32_t*)to)[y*kos_display.lfb_pitch/2+x] = p;
        }
    }
}

static void
copy_display_bpp24_to_rgb888(void *to) {
    uint8_t *from = kos_lfb_base;
    for (size_t y = 0; y < kos_display.height; y++) {
        for (size_t x = 0; x < kos_display.width; x++) {
            uint32_t p = 0;
            p += (uint32_t)from[y*kos_display.width*3+x*3 + 0] << 0;
            p += (uint32_t)from[y*kos_display.width*3+x*3 + 1] << 8;
            p += (uint32_t)from[y*kos_display.width*3+x*3 + 2] << 16;
            ((uint32_t*)to)[y*kos_display.width+x] = p;
        }
    }
}

void
copy_display_bpp32_to_rgb888(void *to) {
    memcpy(to, kos_lfb_base, kos_display.width*kos_display.height*4);
}

void
copy_display_to_rgb888(void *to) {
    switch (kos_display.bits_per_pixel) {
    case 16:
        copy_display_bpp16_to_rgb888(to);
        break;
    case 24:
        copy_display_bpp24_to_rgb888(to);
        break;
    case 32:
        copy_display_bpp32_to_rgb888(to);
        break;
    default:
        fprintf(stderr, "[!] unsupported bit depth: %d\n",
                kos_display.bits_per_pixel);
        break;
    }
}

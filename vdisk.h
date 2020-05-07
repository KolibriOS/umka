#ifndef VDISK_H_INCLUDED
#define VDISK_H_INCLUDED

#include <stdio.h>
#include <inttypes.h>
#include "umka.h"

void *vdisk_init(const char *fname, int adjust_cache_size, size_t cache_size);

__attribute__((__stdcall__))
void vdisk_close(void *userdata);

__attribute__((__stdcall__))
int vdisk_read(void *userdata, void *buffer, off_t startsector,
               size_t *numsectors);

__attribute__((__stdcall__))
int vdisk_write(void *userdata, void *buffer, off_t startsector,
                size_t *numsectors);

__attribute__((__stdcall__))
int vdisk_querymedia(void *userdata, diskmediainfo_t *minfo);

__attribute__((__stdcall__))
unsigned int vdisk_adjust_cache_size(void *userdata, unsigned suggested_size);

extern diskfunc_t vdisk_functions;

#endif  // VDISK_H_INCLUDED

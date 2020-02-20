#ifndef VDISK_H_INCLUDED
#define VDISK_H_INCLUDED

#include <stdio.h>
#include <inttypes.h>
#include "kolibri.h"

void *vdisk_init(const char *fname);

__attribute__((__stdcall__))
void vdisk_close(void *userdata);

__attribute__((__stdcall__))
int vdisk_read(void *userdata, void *buffer, off_t startsector, size_t *numsectors);

__attribute__((__stdcall__))
int vdisk_write(void *userdata, void *buffer, off_t startsector, size_t *numsectors);

__attribute__((__stdcall__))
int vdisk_querymedia(void *userdata, diskmediainfo_t *minfo);

#endif  // VDISK_H_INCLUDED

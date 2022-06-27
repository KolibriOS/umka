#ifndef VDISK_H_INCLUDED
#define VDISK_H_INCLUDED

#include <inttypes.h>
#include "umka.h"

void*
vdisk_init(const char *fname, int adjust_cache_size, size_t cache_size);

extern diskfunc_t vdisk_functions;

#endif  // VDISK_H_INCLUDED

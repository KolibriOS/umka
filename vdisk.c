#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include "kolibri.h"

typedef struct {
    FILE *file;
    uint64_t sect_cnt;
    uint32_t sect_size;
} vdisk_t;

void *vdisk_init(const char *fname) {
    FILE *f = fopen(fname, "r+");
    fseeko(f, 0, SEEK_END);
    off_t fsize = ftello(f);
    fseeko(f, 0, SEEK_SET);
    size_t sect_size = 512;
    if (strstr(fname, "s4096") != NULL || strstr(fname, "s4k") != NULL) {
        sect_size = 4096;
    }
    vdisk_t *vdisk = (vdisk_t*)malloc(sizeof(vdisk_t));
    *vdisk = (vdisk_t){f, (uint64_t)fsize / sect_size, sect_size};
    return vdisk;
}

__attribute__((__stdcall__))
void vdisk_close(void *userdata) {
    vdisk_t *vdisk = userdata;
    fclose(vdisk->file);
    free(vdisk);
}

__attribute__((__stdcall__))
int vdisk_read(void *userdata, void *buffer, off_t startsector, size_t *numsectors) {
    vdisk_t *vdisk = userdata;
    fseeko(vdisk->file, startsector * vdisk->sect_size, SEEK_SET);
    fread(buffer, *numsectors * vdisk->sect_size, 1, vdisk->file);
    return ERROR_SUCCESS;
}

__attribute__((__stdcall__))
int vdisk_write(void *userdata, void *buffer, off_t startsector, size_t *numsectors) {
    vdisk_t *vdisk = userdata;
    fseeko(vdisk->file, startsector * vdisk->sect_size, SEEK_SET);
    fwrite(buffer, *numsectors * vdisk->sect_size, 1, vdisk->file);
    return ERROR_SUCCESS;
}

__attribute__((__stdcall__))
int vdisk_querymedia(void *userdata, diskmediainfo_t *minfo) {
    vdisk_t *vdisk = userdata;
    minfo->flags = 0u;
    minfo->sector_size = vdisk->sect_size;
    minfo->capacity = vdisk->sect_cnt;
    return ERROR_SUCCESS;
}

diskfunc_t vdisk_functions = {
                                 .strucsize = sizeof(diskfunc_t),
                                 .close = vdisk_close,
                                 .closemedia = NULL,
                                 .querymedia = vdisk_querymedia,
                                 .read = vdisk_read,
                                 .write = vdisk_write,
                                 .flush = NULL,
                                 .adjust_cache_size = NULL,
                                };

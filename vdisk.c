#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include <errno.h>
#include "kolibri.h"
#include "trace.h"

typedef struct {
    FILE *file;
    uint32_t sect_size;
    uint64_t sect_cnt;
    unsigned cache_size;
} vdisk_t;

void *vdisk_init(const char *fname, unsigned cache_size) {
    FILE *f = fopen(fname, "r+");
    if (!f) {
        printf("vdisk: can't open file '%s': %s\n", fname, strerror(errno));
        return NULL;
    }
    fseeko(f, 0, SEEK_END);
    off_t fsize = ftello(f);
    fseeko(f, 0, SEEK_SET);
    size_t sect_size = 512;
    if (strstr(fname, "s4096") != NULL || strstr(fname, "s4k") != NULL) {
        sect_size = 4096;
    }
    vdisk_t *vdisk = (vdisk_t*)malloc(sizeof(vdisk_t));
    *vdisk = (vdisk_t){.file = f,
                       .sect_size = sect_size,
                       .sect_cnt = (uint64_t)fsize / sect_size,
                       .cache_size = cache_size};
    return vdisk;
}

__attribute__((__stdcall__))
void vdisk_close(void *userdata) {
    COVERAGE_OFF();
    vdisk_t *vdisk = userdata;
    fclose(vdisk->file);
    free(vdisk);
    COVERAGE_ON();
}

__attribute__((__stdcall__))
int vdisk_read(void *userdata, void *buffer, off_t startsector,
               size_t *numsectors) {
    COVERAGE_OFF();
    vdisk_t *vdisk = userdata;
    fseeko(vdisk->file, startsector * vdisk->sect_size, SEEK_SET);
    fread(buffer, *numsectors * vdisk->sect_size, 1, vdisk->file);
    COVERAGE_ON();
    return ERROR_SUCCESS;
}

__attribute__((__stdcall__))
int vdisk_write(void *userdata, void *buffer, off_t startsector,
                size_t *numsectors) {
    COVERAGE_OFF();
    vdisk_t *vdisk = userdata;
    fseeko(vdisk->file, startsector * vdisk->sect_size, SEEK_SET);
    fwrite(buffer, *numsectors * vdisk->sect_size, 1, vdisk->file);
    COVERAGE_ON();
    return ERROR_SUCCESS;
}

__attribute__((__stdcall__))
int vdisk_querymedia(void *userdata, diskmediainfo_t *minfo) {
    COVERAGE_OFF();
    vdisk_t *vdisk = userdata;
    minfo->flags = 0u;
    minfo->sector_size = vdisk->sect_size;
    minfo->capacity = vdisk->sect_cnt;
    COVERAGE_ON();
    return ERROR_SUCCESS;
}

__attribute__((__stdcall__))
unsigned vdisk_adjust_cache_size(vdisk_t *vdisk, unsigned suggested_size) {
    (void)suggested_size;
    return vdisk->cache_size;
}

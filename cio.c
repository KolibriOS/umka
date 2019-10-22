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

void *cio_disk_init(const char *fname) {
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

void cio_disk_free(vdisk_t *vdisk) {
    fclose(vdisk->file);
    free(vdisk);
}

f70status cio_disk_read(vdisk_t *vdisk, uint8_t *buffer, off_t startsector, uint32_t *numsectors) {
    fseeko(vdisk->file, startsector * vdisk->sect_size, SEEK_SET);
    fread(buffer, *numsectors * vdisk->sect_size, 1, vdisk->file);
    return F70_SUCCESS;
}

f70status cio_disk_write(vdisk_t *vdisk, uint8_t *buffer, off_t startsector, uint32_t *numsectors) {
    fseeko(vdisk->file, startsector * vdisk->sect_size, SEEK_SET);
    fwrite(buffer, *numsectors * vdisk->sect_size, 1, vdisk->file);
    return F70_SUCCESS;
}

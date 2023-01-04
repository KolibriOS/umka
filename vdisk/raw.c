#include <errno.h>
#include <stdlib.h>
#include "../trace.h"
#include "raw.h"

#ifdef _WIN32
#define fseeko _fseeki64
#define ftello _ftelli64
#endif

STDCALL void
vdisk_raw_close(void *userdata) {
    COVERAGE_OFF();
    struct vdisk_raw *disk = userdata;
    fclose(disk->file);
    free(disk);
    COVERAGE_ON();
}

STDCALL int
vdisk_raw_read(void *userdata, void *buffer, off_t startsector,
               size_t *numsectors) {
    COVERAGE_OFF();
    struct vdisk_raw *disk = userdata;
    fseeko(disk->file, startsector * disk->vdisk.sect_size, SEEK_SET);
    fread(buffer, *numsectors * disk->vdisk.sect_size, 1, disk->file);
    COVERAGE_ON();
    return ERROR_SUCCESS;
}

STDCALL int
vdisk_raw_write(void *userdata, void *buffer, off_t startsector,
                size_t *numsectors) {
    COVERAGE_OFF();
    struct vdisk_raw *disk = userdata;
    fseeko(disk->file, startsector * disk->vdisk.sect_size, SEEK_SET);
    fwrite(buffer, *numsectors * disk->vdisk.sect_size, 1, disk->file);
    COVERAGE_ON();
    return ERROR_SUCCESS;
}

struct vdisk_raw*
vdisk_init_raw(const char *fname) {
    FILE *f = fopen(fname, "r+");
    if (!f) {
        printf("[vdisk.raw]: can't open file '%s': %s\n", fname, strerror(errno));
        return NULL;
    }
    fseeko(f, 0, SEEK_END);
    off_t fsize = ftello(f);
    fseeko(f, 0, SEEK_SET);
    size_t sect_size = 512;
    if (strstr(fname, "s4096") != NULL || strstr(fname, "s4k") != NULL) {
        sect_size = 4096;
    }
    struct vdisk_raw *disk = (struct vdisk_raw*)malloc(sizeof(struct vdisk_raw));
    *disk = (struct vdisk_raw){
            .vdisk = {.diskfunc = {.strucsize = sizeof(diskfunc_t),
                                   .close = vdisk_raw_close,
                                   .read = vdisk_raw_read,
                                   .write = vdisk_raw_write,
                                  },
                      .sect_size = sect_size,
                      .sect_cnt = (uint64_t)fsize / sect_size,
                     },
            .file = f,
            };
    return disk;
}

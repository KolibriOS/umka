#include <errno.h>
#include <stdlib.h>
#include "../trace.h"
#include "qcow2.h"
#include "miniz/miniz.h"

struct vdisk_qcow2*
vdisk_init_qcow2(const char *fname) {
    FILE *f = fopen(fname, "r+");
    if (!f) {
        printf("[vdisk.qcow2]: can't open file '%s': %s\n", fname, strerror(errno));
        return NULL;
    }
    return NULL;
}

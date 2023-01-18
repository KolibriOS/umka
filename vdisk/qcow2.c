/*
    SPDX-License-Identifier: GPL-2.0-or-later

    UMKa - User-Mode KolibriOS developer tools
    vdisk - virtual disk, qcow2 format

    Copyright (C) 2023  Ivan Baravy <dunkaist@gmail.com>
*/

#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include "../trace.h"
#include "qcow2.h"
#include "miniz/miniz.h"
#include "io.h"

#define L1_MAX_LEN (32u*1024u*1024u)
#define L1_MAX_ENTRIES (L1_MAX_LEN / sizeof(uint64_t))

struct vdisk_qcow2 {
    struct vdisk vdisk;
    int fd;
    size_t cluster_bits;
    size_t cluster_size;
    uint8_t *cluster;
    uint8_t *cmp_cluster;
    uint64_t l1_table_offset;
    uint64_t l2_entry_cmp_x;
    uint64_t l2_entry_cmp_offset_mask;
    uint64_t l2_entry_cmp_sect_cnt_mask;
    size_t header_length;
    size_t refcount_order;
    size_t refcount_table_clusters;
    off_t refcount_table_offset;
    size_t l1_size;
    uint64_t sector_idx_mask;
    uint64_t *l1;
    uint64_t prev_cluster_index;
};

#define QCOW2_MAGIC "QFI\xfb"

struct qcow2_header {
    char magic[4];
    uint32_t version;
    uint64_t back_file_offset;
    uint32_t back_file_size;
    uint32_t cluster_bits;
    uint64_t size;
    uint32_t crypt_method;
    uint32_t l1_size;
    uint64_t l1_table_offset;
    uint64_t refcount_table_offset;
    uint32_t refcount_table_clusters;
    uint32_t nb_snapshots;
    uint64_t snapshots_offset;
    uint64_t incompatible_features;
    uint64_t compatible_features;
    uint64_t autoclear_features;
    uint32_t refcount_order;
    uint32_t header_length;
};

#define CLUSTER_FORMAT_STANDARD 0
#define CLUSTER_FORMAT_COMPRESSED 1

#define L1_ENTRY_OFFSET_MASK 0x00ffffffffffff00ULL
#define L1_ENTRY_STATUS_MASK 0x8000000000000000ULL

#define L2_ENTRY_STD_ZEROED 0x1ULL
#define L2_ENTRY_STD_OFFSET 0x00ffffffffffff00ULL
#define L2_ENTRY_FORMAT 0x4000000000000000ULL
#define L2_ENTRY_STATUS 0x8000000000000000ULL

static inline uint32_t
be32(void *p) {
    uint8_t *x = p;
    return ((uint32_t)x[3] << 0) + ((uint64_t)x[2] << 8)
           + ((uint64_t)x[1] << 16) + ((uint64_t)x[0] << 24);
}

static inline uint64_t
be64(void *p) {
    uint8_t *x = p;
    return ((uint64_t)x[7] << 0) + ((uint64_t)x[6] << 8)
           + ((uint64_t)x[5] << 16) + ((uint64_t)x[4] << 24)
           + ((uint64_t)x[3] << 32) + ((uint64_t)x[2] << 40)
           + ((uint64_t)x[1] << 48) + ((uint64_t)x[0] << 56);
}

static void
qcow2_read_guest_sector(struct vdisk_qcow2 *d, uint64_t sector, uint8_t *buf) {
    uint64_t cluster_offset;
    size_t l2_entries = d->cluster_size / sizeof(uint64_t);
    uint64_t offset = sector * d->vdisk.sect_size;
    uint64_t cluster_index = offset / d->cluster_size;
    uint64_t l1_index = (cluster_index) / l2_entries;
    uint64_t l2_index = (cluster_index) % l2_entries;
    uint64_t l1_entry = d->l1[l1_index];
    uint64_t l2_entry;

    if (cluster_index == d->prev_cluster_index) {
        memcpy(buf,
               d->cluster + (sector & d->sector_idx_mask) * d->vdisk.sect_size,
               d->vdisk.sect_size);
        return;
    }
    d->prev_cluster_index = cluster_index;

    uint64_t l2_table_offset = l1_entry & L1_ENTRY_OFFSET_MASK;
    if (!l2_table_offset) {
        memset(buf, 0, d->vdisk.sect_size);
        return;
    }
    lseek(d->fd, l2_table_offset + l2_index*sizeof(l2_entry), SEEK_SET);
    if (!io_read(d->fd, &l2_entry, sizeof(l2_entry), d->vdisk.io)) {
        fprintf(stderr, "[vdisk.qcow2] can't read from image file: %s\n",
                strerror(errno));
        return;
    }
    l2_entry = be64(&l2_entry);
    if ((l2_entry & L2_ENTRY_FORMAT) == CLUSTER_FORMAT_STANDARD) {
        if (l2_entry & L2_ENTRY_STD_ZEROED) {
            printf("[vdisk.qcow2] cluster 0x%" PRIx64 " is zeroed\n",
                   cluster_index);
            memset(buf, 0, d->vdisk.sect_size);
            return;
        }
        cluster_offset = l2_entry & L2_ENTRY_STD_OFFSET;
        lseek(d->fd, cluster_offset, SEEK_SET);
        if (!io_read(d->fd, d->cluster, d->cluster_size, d->vdisk.io)) {
            fprintf(stderr, "[vdisk.qcow2] can't read from image file: %s\n",
                    strerror(errno));
            return;
        }
    } else {
        off_t cmp_offset = d->l2_entry_cmp_offset_mask & l2_entry;
        printf("cmp_offset: 0x%" PRIx64 "\n", cmp_offset);
        lseek(d->fd, cmp_offset, SEEK_SET);
        size_t additional_sectors = (l2_entry & d->l2_entry_cmp_sect_cnt_mask)
                                    >> d->l2_entry_cmp_x;
        size_t cmp_size = 512 - (cmp_offset & 511) + additional_sectors*512;
        if (!io_read(d->fd, d->cmp_cluster, d->cluster_size, d->vdisk.io)) {
            fprintf(stderr, "[vdisk.qcow2] can't read from image file: %s\n",
                    strerror(errno));
            return;
        }
        unsigned long dest_size = d->cluster_size;
        uncompress(d->cluster, &dest_size, d->cmp_cluster, cmp_size);
    }
    memcpy(buf,
           d->cluster + (sector & d->sector_idx_mask) * d->vdisk.sect_size,
           d->vdisk.sect_size);
}

STDCALL void
vdisk_qcow2_close(void *userdata) {
    COVERAGE_OFF();
    struct vdisk_qcow2 *d = userdata;
    if (d->fd) {
        close(d->fd);
    }
    free(d->cluster);
    free(d->cmp_cluster);
    free(d->l1);
    free(d);
    COVERAGE_ON();
}

STDCALL int
vdisk_qcow2_read(void *userdata, void *buffer, off_t startsector,
                 size_t *numsectors) {
    COVERAGE_OFF();
    struct vdisk_qcow2 *d = userdata;
    for (size_t i = 0; i < *numsectors; i++) {
        qcow2_read_guest_sector(d, startsector + i, buffer);
        buffer = (uint8_t*)buffer + d->vdisk.sect_size;
    }
    COVERAGE_ON();
    return ERROR_SUCCESS;
}

STDCALL int
vdisk_qcow2_write(void *userdata, void *buffer, off_t startsector,
                  size_t *numsectors) {
    COVERAGE_OFF();
    struct vdisk_qcow2 *d = userdata;
    (void)d;
    (void)buffer;
    (void)startsector;
    (void)numsectors;
    fprintf(stderr, "[vdisk.qcow2] writing is not implemented");
    COVERAGE_ON();
    return ERROR_UNSUPPORTED_FS;
}

struct vdisk*
vdisk_init_qcow2(const char *fname, struct umka_io *io) {
    struct vdisk_qcow2 *d =
        (struct vdisk_qcow2*)calloc(1, sizeof(struct vdisk_qcow2));
    if (!d) {
        fprintf(stderr, "[vdisk.qcow2] can't allocate memory: %s\n",
                strerror(errno));
        return NULL;
    }

    d->vdisk.diskfunc = (diskfunc_t) {.strucsize = sizeof(diskfunc_t),
                                      .close = vdisk_qcow2_close,
                                      .read = vdisk_qcow2_read,
                                      .write = vdisk_qcow2_write,
                                     };
    d->vdisk.io = io;
    d->prev_cluster_index = ~(uint64_t)0;
    if (!(d->fd = open(fname, O_RDONLY))) {
        fprintf(stderr, "[vdisk.qcow2] can't open file '%s': %s\n", fname,
                strerror(errno));
        vdisk_qcow2_close(d);
        return NULL;
    }
    d->vdisk.sect_size = 512;
    if (strstr(fname, "s4096") != NULL || strstr(fname, "s4k") != NULL) {
        d->vdisk.sect_size = 4096;
    }

    struct qcow2_header header;
    if (!io_read(d->fd, &header, sizeof(struct qcow2_header), d->vdisk.io)) {
        fprintf(stderr, "[vdisk.qcow2] can't read from image file: %s\n",
                strerror(errno));
        vdisk_qcow2_close(d);
        return NULL;
    }

    if (strncmp(header.magic, QCOW2_MAGIC, sizeof(header.magic))) {
        fprintf(stderr, "[vdisk.qcow2] bad image signature: '%c%c%c%c'\n",
                header.magic[0], header.magic[1], header.magic[2],
                header.magic[3]);
        vdisk_qcow2_close(d);
        return NULL;
    }

    uint32_t version = be32(&header.version);
    if (version != 3) {
        fprintf(stderr, "[vdisk.qcow2] bad image format version: %" PRIu32 "\n",
                version);
        vdisk_qcow2_close(d);
        return NULL;
    }

    d->cluster_bits = be32(&header.cluster_bits);
    if (d->cluster_bits < 9 || d->cluster_bits > 21) {
        fprintf(stderr, "[vdisk.qcow2] bad cluster_bits value: %u\n",
                d->cluster_bits);
        vdisk_qcow2_close(d);
        return NULL;
    }
    d->cluster_size = 1 << d->cluster_bits;
    d->sector_idx_mask = d->cluster_size / d->vdisk.sect_size - 1ULL;

    d->l2_entry_cmp_x = 62 - (d->cluster_bits - 8);
    d->l2_entry_cmp_offset_mask = (1ULL << d->l2_entry_cmp_x) - 1ULL;
    d->l2_entry_cmp_sect_cnt_mask = ((1ULL << 62) - 1ULL)
                                    ^ d->l2_entry_cmp_offset_mask;

    uint64_t size = be64(&header.size);
    d->vdisk.sect_cnt = size / d->vdisk.sect_size;
    uint32_t crypt_method = be32(&header.crypt_method);
    if (crypt_method) {
        fprintf(stderr, "[vdisk.qcow2] bad crypt_method: %u\n", crypt_method);
        vdisk_qcow2_close(d);
        return NULL;
    }

    d->l1_size = be32(&header.l1_size);
    d->l1_table_offset = be64(&header.l1_table_offset);
    d->refcount_table_offset = be64(&header.refcount_table_offset);
    d->refcount_table_clusters = be32(&header.refcount_table_clusters);

    uint64_t incompatible_features = be64(&header.incompatible_features);
    if (incompatible_features) {
        fprintf(stderr, "[vdisk.qcow2] unsupported incompatible_feature(s): 0x%"
                PRIx64 "\n", incompatible_features);
        vdisk_qcow2_close(d);
        return NULL;
    }

    d->refcount_order = be32(&header.refcount_order);
    if (d->refcount_order < 4 || d->refcount_order > 6) {
        fprintf(stderr, "[vdisk.qcow2] bad refcount_order value: %u\n",
                d->refcount_order);
        vdisk_qcow2_close(d);
        return NULL;
    }

    d->header_length = be32(&header.header_length);
    d->cluster = (uint8_t*)malloc(d->cluster_size);
    if (!d->cluster) {
        fprintf(stderr, "[vdisk.qcow2] can't allocate memory: %s\n",
                strerror(errno));
        vdisk_qcow2_close(d);
        return NULL;
    }

    d->cmp_cluster = (uint8_t*)malloc(d->cluster_size*2);
    if (!d->cmp_cluster) {
        fprintf(stderr, "[vdisk.qcow2] can't allocate memory: %s\n",
                strerror(errno));
        vdisk_qcow2_close(d);
        return NULL;
    }

    d->l1 = (uint64_t*)malloc(d->l1_size * sizeof(uint64_t));
    if (!d->l1) {
        fprintf(stderr, "[vdisk.qcow2] can't allocate memory: %s\n",
                strerror(errno));
        vdisk_qcow2_close(d);
        return NULL;
    }

    lseek(d->fd, d->l1_table_offset, SEEK_SET);
    if (!io_read(d->fd, d->l1, d->l1_size * sizeof(uint64_t), d->vdisk.io)) {
        fprintf(stderr, "[vdisk.qcow2] can't read from image file: %s\n",
                strerror(errno));
        vdisk_qcow2_close(d);
        return NULL;
    }

    for (uint64_t *x = d->l1; x < d->l1 + d->l1_size; x++) {
        *x = be64(x);
    }

    return (struct vdisk*)d;
}

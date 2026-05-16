/*
    SPDX-License-Identifier: GPL-2.0-or-later

    UMKa - User-Mode KolibriOS developer tools
    vdisk - virtual disk, qcow2 format

    Copyright (C) 2023,2025-2026  Ivan Baravy <dunkaist@gmail.com>
*/

#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include "../trace.h"
#include "qcow2.h"
#include "umkaio.h"
#include "em_inflate/em_inflate.h"

#define QCOW2_DIRTY_SUFFIX ".dirty"

struct qcow2_file {
    char *fname;
    int fd;
    uint64_t *l1;
};

struct vdisk_qcow2 {
    struct vdisk vdisk;
    struct qcow2_file base;
    struct qcow2_file dirty;
    size_t cluster_bits;
    size_t cluster_size;
    uint8_t *cluster;
    uint8_t *cmp_cluster;
    uint64_t l1_table_offset;
    uint64_t l2_entry_cmp_x;
    uint64_t l2_entry_cmp_offset_mask;
    uint64_t l2_entry_cmp_sect_cnt_mask;
    size_t l1_size;
    uint64_t sector_idx_mask;
    uint64_t prev_cluster_index;
};

#define QCOW2_MAGIC "QFI\xfb"

struct qcow2_header {
    [[gnu::nonstring]] char magic[4];
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

#define BSWAP32(x) ( (((uint32_t)(x) & 0x000000ffu) << 24) \
                    + (((uint32_t)(x) & 0x0000ff00u) << 8) \
                    + (((uint32_t)(x) & 0x00ff0000u) >> 8) \
                    + (((uint32_t)(x) & 0xff000000u) >> 24))

#define BSWAP64(x) ( (((uint64_t)(x) & 0x00000000000000ffull) << 56) \
                    + (((uint64_t)(x) & 0x000000000000ff00ull) << 40) \
                    + (((uint64_t)(x) & 0x0000000000ff0000ull) << 24) \
                    + (((uint64_t)(x) & 0x00000000ff000000ull) << 8) \
                    + (((uint64_t)(x) & 0x000000ff00000000ull) >> 8) \
                    + (((uint64_t)(x) & 0x0000ff0000000000ull) >> 24) \
                    + (((uint64_t)(x) & 0x00ff000000000000ull) >> 40) \
                    + (((uint64_t)(x) & 0xff00000000000000ull) >> 56))

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

static int
qcow2_read_guest_cluster(struct vdisk_qcow2 *d, struct qcow2_file *qf,
                         uint64_t cluster_index) {
    uint64_t cluster_offset;
    size_t l2_entries = d->cluster_size / sizeof(uint64_t);
    uint64_t l1_index = (cluster_index) / l2_entries;
    uint64_t l2_index = (cluster_index) % l2_entries;
    uint64_t l1_entry = qf->l1[l1_index];    // TODO: move all l2 to mem
    uint64_t l2_entry;

    uint64_t l2_table_offset = l1_entry & L1_ENTRY_OFFSET_MASK;
    if (!l2_table_offset) {
        return 0;
    }
    lseek(qf->fd, l2_table_offset + l2_index*sizeof(l2_entry), SEEK_SET);
    if (!io_read(qf->fd, &l2_entry, sizeof(l2_entry), d->vdisk.io)) {
        fprintf(stderr, "[vdisk.qcow2] can't read from image file: %s\n",
                strerror(errno));
        return -1;
    }
    l2_entry = be64(&l2_entry);
    if ((l2_entry & L2_ENTRY_FORMAT) == CLUSTER_FORMAT_STANDARD) {
        cluster_offset = l2_entry & L2_ENTRY_STD_OFFSET;
        if (!cluster_offset) {
            return 0;
        }
        lseek(qf->fd, cluster_offset, SEEK_SET);
        if (!io_read(qf->fd, d->cluster, d->cluster_size, d->vdisk.io)) {
            fprintf(stderr, "[vdisk.qcow2] can't read from image file: %s\n",
                    strerror(errno));
            return -1;
        }
    } else {
        // compressed
        off_t cmp_offset = d->l2_entry_cmp_offset_mask & l2_entry;
        printf("cmp_offset: 0x%" PRIx64 "\n", cmp_offset);
        lseek(qf->fd, cmp_offset, SEEK_SET);
        size_t additional_sectors = (l2_entry & d->l2_entry_cmp_sect_cnt_mask)
                                    >> d->l2_entry_cmp_x;
        size_t cmp_size = 512 - (cmp_offset & 511) + additional_sectors*512;
        if (!io_read(qf->fd, d->cmp_cluster, d->cluster_size, d->vdisk.io)) {
            fprintf(stderr, "[vdisk.qcow2] can't read from image file: %s\n",
                    strerror(errno));
            return -1;
        }
        unsigned long dest_size = d->cluster_size;
        em_inflate(d->cluster, dest_size, d->cmp_cluster, cmp_size);
    }
    return 1;
}

static int
qcow2_read_guest_sector(struct vdisk_qcow2 *d, uint64_t sector, uint8_t *buf) {
    uint64_t offset = sector * d->vdisk.sect_size;
    uint64_t cluster_index = offset / d->cluster_size;

    int status = 0;
    if (d->dirty.fd) {
        status = qcow2_read_guest_cluster(d, &d->dirty, cluster_index);
    }
    if (!status) {
        status = qcow2_read_guest_cluster(d, &d->base, cluster_index);
    }
    if (status < 0) {
        fprintf(stderr, "[vdisk.qcow2] can't read from image file: %s\n",
                strerror(errno));
        return KOS_ERROR_DEVICE;
    }
    if (!status) {
        memset(buf, 0, d->vdisk.sect_size);
    } else {
        memcpy(buf,
               d->cluster + (sector & d->sector_idx_mask) * d->vdisk.sect_size,
               d->vdisk.sect_size);
    }
    return KOS_ERROR_SUCCESS;
}

static int
qcow2_write_guest_cluster(struct vdisk_qcow2 *d, struct qcow2_file *qf,
                          uint64_t cluster_index) {
    uint64_t cluster_offset;
    size_t l2_entries = d->cluster_size / sizeof(uint64_t);
    uint64_t l1_index = (cluster_index) / l2_entries;
    uint64_t l2_index = (cluster_index) % l2_entries;
    uint64_t l1_entry = qf->l1[l1_index];
    uint64_t l2_entry;

    d->prev_cluster_index = ~(uint64_t)0;

    uint64_t l2_table_offset = l1_entry & L1_ENTRY_OFFSET_MASK;
    if (!l2_table_offset) {
        l2_table_offset = lseek(qf->fd, 0u, SEEK_END);
        qf->l1[l1_index] = l2_table_offset;
        uint8_t *zero_cluster = calloc(1, d->cluster_size);
        write(qf->fd, zero_cluster, d->cluster_size);
        free(zero_cluster);
    }
    lseek(qf->fd, l2_table_offset + l2_index*sizeof(l2_entry), SEEK_SET);
    if (!io_read(qf->fd, &l2_entry, sizeof(l2_entry), d->vdisk.io)) {
        fprintf(stderr, "[vdisk.qcow2] can't read from image file: %s\n",
                strerror(errno));
        return -1;
    }
    l2_entry = be64(&l2_entry);
    cluster_offset = l2_entry & L2_ENTRY_STD_OFFSET;
    if (!cluster_offset) {
        cluster_offset = lseek(qf->fd, 0u, SEEK_END);
        uint64_t cluster_offset_be = BSWAP64(cluster_offset + L2_ENTRY_STATUS);
        lseek(qf->fd, l2_table_offset + l2_index*sizeof(l2_entry), SEEK_SET);
        write(qf->fd, &cluster_offset_be, sizeof(uint64_t));
    }
    lseek(qf->fd, cluster_offset, SEEK_SET);
    if (!io_write(qf->fd, d->cluster, d->cluster_size, d->vdisk.io)) {
        fprintf(stderr, "[vdisk.qcow2] can't write to image file: %s\n",
                strerror(errno));
        return -1;
    }
    return 1;
}

static int
qcow2_write_guest_sector(struct vdisk_qcow2 *d, uint64_t sector, uint8_t *buf) {
    uint64_t offset = sector * d->vdisk.sect_size;
    uint64_t cluster_index = offset / d->cluster_size;

    d->prev_cluster_index = ~(uint64_t)0;   // TODO: rename to cached_cluster_idx

    int status = 0;
    status = qcow2_read_guest_cluster(d, &d->dirty, cluster_index);
    if (!status) {
        status = qcow2_read_guest_cluster(d, &d->base, cluster_index);
    }
    if (!status) {
        memset(d->cluster, 0, d->cluster_size);
    }
    memcpy(d->cluster + (sector & d->sector_idx_mask) * d->vdisk.sect_size,
           buf, d->vdisk.sect_size);
    status = qcow2_write_guest_cluster(d, &d->dirty, cluster_index);
    return 1;
}

[[gnu::stdcall]]
void
vdisk_qcow2_close(void *userdata) {
    COVERAGE_OFF();
    struct vdisk_qcow2 *d = userdata;
    if (d->base.fname) {
        free(d->base.fname);
        if (d->base.fd) {
            close(d->base.fd);
        }
    }
    if (d->dirty.fname) {
        if (d->dirty.fd) {
            lseek(d->dirty.fd, d->l1_table_offset, SEEK_SET);
            for (uint64_t *x = d->dirty.l1; x < d->dirty.l1 + d->l1_size; x++) {
                *x = be64(x);
            }
            write(d->dirty.fd, d->dirty.l1, d->l1_size * sizeof(uint64_t));
            close(d->dirty.fd);
        }
        unlink(d->dirty.fname);
        free(d->dirty.fname);
    }
    free(d->cluster);
    free(d->cmp_cluster);
    free(d->base.l1);
    free(d->dirty.l1);
    free(d);
    COVERAGE_ON();
}

[[gnu::stdcall]]
int
vdisk_qcow2_read(void *userdata, void *buffer, off_t startsector,
                 size_t *numsectors) {
    COVERAGE_OFF();
    struct vdisk_qcow2 *d = userdata;
    int status = KOS_ERROR_SUCCESS;
    for (size_t i = 0; i < *numsectors; i++) {
        status = qcow2_read_guest_sector(d, startsector + i, buffer);
        if (status < 0) {
            fprintf(stderr, "[vdisk.qcow2] can't read from image file: %s\n",
                    strerror(errno));
            break;
        }
        buffer = (uint8_t*)buffer + d->vdisk.sect_size;
    }
    COVERAGE_ON();
    return status;
}

// TODO: change to io_*
static int
qcow2_init_dirty_file(struct vdisk_qcow2 *d) {
    int fd;
    if ((fd = open(d->dirty.fname, O_RDWR | O_BINARY | O_CREAT | O_TRUNC,
                   S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)) == -1) {
        fprintf(stderr, "[vdisk.qcow2] can't open file '%s': %s\n",
                d->dirty.fname, strerror(errno));
    }
    struct qcow2_header h = {.magic = QCOW2_MAGIC,
                             .version = BSWAP32(3u),
                             .back_file_offset = 0u,
                             .back_file_size = 0u,
                             .cluster_bits = BSWAP32(d->cluster_bits),
                             .size = BSWAP64(d->vdisk.sect_cnt
                                             * d->vdisk.sect_size),
                             .crypt_method = 0u,
                             .l1_size = BSWAP32(d->l1_size),
                             .l1_table_offset = BSWAP64(d->l1_table_offset),
                            };
    write(fd, &h, sizeof(h));
    d->dirty.l1 = calloc(d->l1_size, sizeof(uint64_t));
    off_t aligned = d->l1_table_offset + d->l1_size * sizeof(uint64_t);
    aligned += 1u << d->cluster_bits;
    aligned &= ~( (1u << d->cluster_bits) - 1);
    ftruncate(fd, aligned);
    return fd;
}

[[gnu::stdcall]]
int
vdisk_qcow2_write(void *userdata, void *buffer, off_t startsector,
                  size_t *numsectors) {
    COVERAGE_OFF();
    struct vdisk_qcow2 *d = userdata;
    if (!d->dirty.fd) {
        d->dirty.fd = qcow2_init_dirty_file(d);
    }
    for (size_t i = 0; i < *numsectors; i++) {
        int status = qcow2_write_guest_sector(d, startsector + i, buffer);
        if (status < 0) {
            fprintf(stderr, "[vdisk.qcow2] can't read from image file: %s\n",
                    strerror(errno));
        }
        buffer = (uint8_t*)buffer + d->vdisk.sect_size;
    }
    COVERAGE_ON();
    return KOS_ERROR_SUCCESS;
}

struct vdisk*
vdisk_init_qcow2(const char *fname, const struct umka_io *io) {
    struct vdisk_qcow2 *d =
        (struct vdisk_qcow2*)calloc(1, sizeof(struct vdisk_qcow2));
    if (!d) {
        fprintf(stderr, "[vdisk.qcow2] can't allocate memory: %s\n",
                strerror(errno));
        return NULL;
    }

    d->base.fname = strdup(fname);
    d->dirty.fname = malloc(512);
    const char *fbasename = strrchr(fname, '/');
    if (!fbasename) {
        fbasename = fname;
    } else {
        fbasename++;
    }
    sprintf(d->dirty.fname, "%s" QCOW2_DIRTY_SUFFIX, fbasename);

    d->vdisk.diskfunc = (struct diskfunc) {.strucsize = sizeof(struct diskfunc),
                                      .close = vdisk_qcow2_close,
                                      .read = vdisk_qcow2_read,
                                      .write = vdisk_qcow2_write,
                                     };
    d->vdisk.io = io;
    d->prev_cluster_index = ~(uint64_t)0;
    if ((d->base.fd = open(d->base.fname, O_RDONLY | O_BINARY)) == -1) {
        fprintf(stderr, "[vdisk.qcow2] can't open file '%s': %s\n",
                d->base.fname, strerror(errno));
        vdisk_qcow2_close(d);
        return NULL;
    }
    d->vdisk.sect_size = 512;
    if ((strstr(d->base.fname, "s4096") != NULL)
        || (strstr(d->base.fname, "s4k") != NULL)) {
        d->vdisk.sect_size = 4096;
    } else if ((strstr(d->base.fname, "s2048") != NULL)
               || (strstr(d->base.fname, "s2k") != NULL)) {
        d->vdisk.sect_size = 2048;
    }

    struct qcow2_header header;
    if (!io_read(d->base.fd, &header, sizeof(struct qcow2_header), d->vdisk.io)) {
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

    uint64_t incompatible_features = be64(&header.incompatible_features);
    if (incompatible_features) {
        fprintf(stderr, "[vdisk.qcow2] unsupported incompatible_feature(s): 0x%"
                PRIx64 "\n", incompatible_features);
        vdisk_qcow2_close(d);
        return NULL;
    }

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

    d->base.l1 = (uint64_t*)malloc(d->l1_size * sizeof(uint64_t));
    if (!d->base.l1) {
        fprintf(stderr, "[vdisk.qcow2] can't allocate memory: %s\n",
                strerror(errno));
        vdisk_qcow2_close(d);
        return NULL;
    }

    lseek(d->base.fd, d->l1_table_offset, SEEK_SET);
    if (!io_read(d->base.fd, d->base.l1, d->l1_size * sizeof(uint64_t), d->vdisk.io)) {
        fprintf(stderr, "[vdisk.qcow2] can't read from image file: %s\n",
                strerror(errno));
        vdisk_qcow2_close(d);
        return NULL;
    }

    for (uint64_t *x = d->base.l1; x < d->base.l1 + d->l1_size; x++) {
        *x = be64(x);
    }

    return (struct vdisk*)d;
}

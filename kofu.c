#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <inttypes.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <assert.h>
#include "kolibri.h"
#include "trace.h"

// http://home.thep.lu.se/~bjorn/crc/
/* Simple public domain implementation of the standard CRC32 checksum.
 * Outputs the checksum for each file given as a command line argument.
 * Invalid file names and files that cause errors are silently skipped.
 * The program reads from stdin if it is called with no arguments. */

uint32_t crc32_for_byte(uint32_t r) {
  for(int j = 0; j < 8; ++j)
    r = (r & 1? 0: (uint32_t)0xEDB88320L) ^ r >> 1;
  return r ^ (uint32_t)0xFF000000L;
}

void crc32(const void *data, size_t n_bytes, uint32_t* crc) {
  static uint32_t table[0x100];
  if(!*table)
    for(size_t i = 0; i < 0x100; ++i)
      table[i] = crc32_for_byte(i);
  for(size_t i = 0; i < n_bytes; ++i)
    *crc = table[(uint8_t)*crc ^ ((uint8_t*)data)[i]] ^ *crc >> 8;
}

#define FGETS_BUF_LEN 4096
#define MAX_COMMAND_ARGS 42
#define PRINT_BYTES_PER_LINE 32
#define MAX_DIRENTS_TO_READ 100
#define MAX_BYTES_TO_READ 1024

char cmd_buf[FGETS_BUF_LEN];
bool is_tty;
int cmd_num = 0;
bool trace = false;

void prompt() {
    printf("#%d> ", cmd_num);
    fflush(stdout);
}

bool next_line() {
    if (is_tty) {
        prompt();
    }
    return fgets(cmd_buf, FGETS_BUF_LEN, stdin) != NULL;
}

int split_args(char *s, const char **argv) {
    int argc = -1;
    for ( ; (argv[++argc] = strtok(s, " \t\n")) != NULL; s = NULL );
    return argc;
}

void kofu_disk_add(int argc, const char **argv) {
    (void)argc;
    const char *file_name = argv[1];
    const char *disk_name = argv[2];
    if (kos_disk_add(file_name, disk_name)) {
        printf("[!!] can't add file '%s' as disk '%s'\n", file_name, disk_name);
    }
    return;
}

void kofu_disk_del(int argc, const char **argv) {
    (void)argc;
    const char *name = argv[1];
    if (kos_disk_del(name)) {
        printf("[!!] can't find or delete disk '%s'\n", name);
    }
    return;
}

void ls_range(struct f70s1arg *f70) {
    f70ret r;
    uint32_t requested = f70->size;
    if (f70->size > MAX_DIRENTS_TO_READ) {
        f70->size = MAX_DIRENTS_TO_READ;
    }
    for (; requested; requested -= f70->size) {
        if (f70->size > requested) {
            f70->size = requested;
        }
        kos_fuse_lfn(f70, &r);
        f70->offset += f70->size;
        printf("status = %d, count = %d\n", r.status, r.count);
        struct f70s1ret *dir = f70->buf;
        assert((r.status == F70_SUCCESS && r.count == f70->size) ||
               (r.status == F70_END_OF_FILE && r.count < f70->size)
              );
        assert(dir->cnt == r.count);
        if (dir->cnt != r.count ||
            (r.status == F70_SUCCESS && r.count != f70->size) ||
            (r.status == F70_END_OF_FILE && r.count >= f70->size)
           ) {
            abort();
        }
        for (size_t i = 0; i < dir->cnt; i++) {
            printf("%s\n", dir->bdfes[i].name);
        }
        if (r.status == F70_END_OF_FILE) {
            break;
        }
    }
}

void ls_all(struct f70s1arg *f70) {
    f70ret r;
    while (true) {
        kos_fuse_lfn(f70, &r);
        printf("status = %d, count = %d\n", r.status, r.count);
        if (r.status != F70_SUCCESS && r.status != F70_END_OF_FILE) {
            abort();
        }
        struct f70s1ret *dir = f70->buf;
        f70->offset += dir->cnt;
        assert((r.status == F70_SUCCESS && r.count == f70->size) ||
               (r.status == F70_END_OF_FILE && r.count < f70->size)
              );
        assert(dir->cnt == r.count);
        for (size_t i = 0; i < dir->cnt; i++) {
            printf("%s\n", dir->bdfes[i].name);
        }
        if (r.status == F70_END_OF_FILE) {
            break;
        }
    }
}

void kofu_ls(int argc, const char **argv) {
    (void)argc;
    struct f70s1ret *dir = (struct f70s1ret*)malloc(sizeof(struct f70s1ret) + sizeof(struct bdfe) * MAX_DIRENTS_TO_READ);
    struct f70s1arg f70 = {1, 0, CP866, MAX_DIRENTS_TO_READ, dir, 0, argv[1]};
    if (argv[2]) {
        sscanf(argv[2], "%"SCNu32, &f70.size);
        if (argv[3]) {
            sscanf(argv[3], "%"SCNu32, &f70.offset);
        }
        ls_range(&f70);
    } else {
        ls_all(&f70);
    }
    free(dir);
    return;
}

void kofu_stat(int argc, const char **argv) {
    (void)argc;
    f70ret r;
    struct bdfe file;
    struct f70s5arg f70 = {5, 0, 0, 0, &file, 0, argv[1]};
    kos_fuse_lfn(&f70, &r);
    printf("attr: 0x%2.2x\n", file.attr);
    printf("size: %llu\n", file.size);
    return;
}

void read_range(struct f70s0arg *f70) {
    f70ret r;
    uint32_t requested = f70->size;
    if (f70->size > MAX_BYTES_TO_READ) {
        f70->size = MAX_BYTES_TO_READ;
    }
    for (; requested; requested -= f70->size) {
        if (f70->size > requested) {
            f70->size = requested;
        }
        kos_fuse_lfn(f70, &r);
        f70->offset_lo += f70->size;
        printf("status = %d, count = %d\n", r.status, r.count);
        assert((r.status == F70_SUCCESS && r.count == f70->size) ||
               (r.status == F70_END_OF_FILE && r.count < f70->size)
              );
        for (size_t i = 0; i < r.count; i++) {
            if (i % PRINT_BYTES_PER_LINE == 0 && i != 0) {
                printf("\n");
            }
            printf("%2.2x", ((uint8_t*)f70->buf)[i]);
        }
        printf("\n");
        if (r.status == F70_END_OF_FILE) {
            break;
        }
    }
}

void read_all(struct f70s0arg *f70) {
    f70ret r;
    while (true) {
        kos_fuse_lfn(f70, &r);
        f70->offset_lo += f70->size;
        printf("status = %d, count = %d\n", r.status, r.count);
        assert((r.status == F70_SUCCESS && r.count == f70->size) ||
               (r.status == F70_END_OF_FILE && r.count < f70->size)
              );
        for (size_t i = 0; i < r.count; i++) {
            if (i % PRINT_BYTES_PER_LINE == 0 && i != 0) {
                printf("\n");
            }
            printf("%2.2x", ((uint8_t*)f70->buf)[i]);
        }
        printf("\n");
        if (r.status == F70_END_OF_FILE) {
            break;
        }
    }
}

void kofu_read(int argc, const char **argv) {
    (void)argc;
    uint8_t *buf = (uint8_t*)malloc(MAX_BYTES_TO_READ);
    struct f70s0arg f70 = {0, 0, 0, MAX_BYTES_TO_READ, buf, 0, argv[1]};
//    optind = 1;
//    while ((opt = getopt(argc, argv, "fcshd:")) != -1) {
//        switch (opt) {
//        case
    if (argv[2]) {
        sscanf(argv[2], "%"SCNu32, &f70.size);
        if (argv[3]) {
            sscanf(argv[3], "%"SCNu32, &f70.offset_lo);
        }
        read_range(&f70);
    } else {
        read_all(&f70);
    }
    free(buf);
    return;
}

struct func_table {
    char *name;
    void (*func) (int, const char **);
};

struct func_table funcs[] = {
                              { "disk_add", kofu_disk_add },
                              { "disk_del", kofu_disk_del },
                              { "ls",   kofu_ls   },
                              { "stat", kofu_stat },
                              { "read", kofu_read },
                              { NULL,   NULL      },
                            };

void crc() {
    uint8_t data[] = {1,2,3,4};
    uint32_t x = 0;
    crc32(data, 4, &x);
    printf("crc=%"PRIx32"\n", x);
}

int main(int argc, char **argv) {
    (void)argc;
    (void)argv;
/*
    if (argc != 2) {
        printf("usage: kofu <file.xfs>\n");
        exit(1);
    }
*/
    is_tty = isatty(STDIN_FILENO);

    if (trace) {
        trace_begin();
    }
    kos_init();
//    kos_disk_add(argv[1], "hd0");
//    if (cio_init(argv[1])) {
//        exit(1);
//    }

//msg_file_not_found       db 'file not found: '
    while(next_line()) {
        if (!is_tty) {
            prompt();
            printf("%s", cmd_buf);
            fflush(stdout);
        }
        const char **cargv = (const char**)malloc(sizeof(char*) * (MAX_COMMAND_ARGS + 1));
        int cargc = split_args(cmd_buf, cargv);
        bool found = false;
        for (struct func_table *ft = funcs; ft->name != NULL; ft++) {
            if (!strcmp(cargv[0], ft->name)) {
                found = true;
                ft->func(cargc, cargv);
                cmd_num++;
                break;
            }
        }
        if (!found) {
            printf("unknown command: %s\n", cargv[0]);
        }
    }

    if (trace) {
        trace_end();
    }
    return 0;
}

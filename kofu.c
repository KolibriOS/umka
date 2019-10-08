#define _GNU_SOURCE
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <inttypes.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <assert.h>
#include "kolibri.h"
#include "trace.h"

#define FGETS_BUF_LEN 4096
#define MAX_COMMAND_ARGS 42
#define PRINT_BYTES_PER_LINE 32
#define MAX_DIRENTS_TO_READ 100
#define MAX_BYTES_TO_READ 1024

char cmd_buf[FGETS_BUF_LEN];
bool is_tty;
int cmd_num = 0;

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

char **split_args(char *s) {
    int argc = -1;
    char **argv = (char**)malloc(sizeof(char*) * (MAX_COMMAND_ARGS + 1));
    char *saveptr;
    for ( ; (argv[++argc] = strtok_r(s, " \t\n", &saveptr)) != NULL; s = NULL );
    return argv;
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

void kofu_ls(char **arg) {
    struct f70s1ret *dir = (struct f70s1ret*)malloc(sizeof(struct f70s1ret) + sizeof(struct bdfe) * MAX_DIRENTS_TO_READ);
    struct f70s1arg f70 = {1, 0, CP866, MAX_DIRENTS_TO_READ, dir, 0, arg[1]};
    if (arg[2]) {
        sscanf(arg[2], "%"SCNu32, &f70.size);
        if (arg[3]) {
            sscanf(arg[3], "%"SCNu32, &f70.offset);
        }
        ls_range(&f70);
    } else {
        ls_all(&f70);
    }
    free(dir);
    return;
}

void kofu_stat(char **arg) {
    f70ret r;
    struct bdfe file;
    struct f70s5arg f70 = {5, 0, 0, 0, &file, 0, arg[1]};
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

void kofu_read(char **arg) {
    uint8_t *buf = (uint8_t*)malloc(MAX_BYTES_TO_READ);
    struct f70s0arg f70 = {0, 0, 0, MAX_BYTES_TO_READ, buf, 0, arg[1]};
    if (arg[2]) {
        sscanf(arg[2], "%"SCNu32, &f70.size);
        if (arg[3]) {
            sscanf(arg[3], "%"SCNu32, &f70.offset_lo);
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
    void (*func) (char **arg);
};

struct func_table funcs[] = {
                              { "ls",   kofu_ls   },
                              { "stat", kofu_stat },
                              { "read", kofu_read },
                              { NULL,   NULL      },
                            };

int main(int argc, char **argv) {
    if (argc != 2) {
        printf("usage: kofu <file.xfs>\n");
        exit(1);
    }
    is_tty = isatty(STDIN_FILENO);

    int fd = open(argv[1], O_RDONLY);
    struct stat st;
    fstat(fd, &st);
    trace_begin();
    if (!kos_fuse_init(fd, st.st_size / 512, 512)) {
        exit(1);
    }

//msg_file_not_found       db 'file not found: '
    while(next_line()) {
        if (!is_tty) {
            prompt();
            printf("%s", cmd_buf);
        }
        char **arg = split_args(cmd_buf);
        bool found = false;
        for (struct func_table *ft = funcs; ft->name != NULL; ft++) {
            if (!strcmp(arg[0], ft->name)) {
                found = true;
                ft->func(arg);
                cmd_num++;
                break;
            }
        }
        if (!found) {
            printf("unknown command: %s\n", arg[0]);
        }
    }

    trace_end();
    return 0;
}

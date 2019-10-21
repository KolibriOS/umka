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
#define MAX_BYTES_TO_READ (16*1024)

char cmd_buf[FGETS_BUF_LEN];
bool is_tty;
int cmd_num = 0;
bool trace = false;

bool parse_uintmax(const char *str, uintmax_t *res) {
    char *endptr;
    *res = strtoumax(str, &endptr, 0);
    bool ok = (str != endptr) && (*endptr == '\0');
    return ok;
}

bool parse_uint32(const char *str, uint32_t *res) {
    uintmax_t x;
    if (parse_uintmax(str, &x) && x <= UINT32_MAX) {
        *res = x;
        return true;
    } else {
        perror("invalid number");
        return false;
    }
}

bool parse_uint64(const char *str, uint64_t *res) {
    uintmax_t x;
    if (parse_uintmax(str, &x) && x <= UINT64_MAX) {
        *res = x;
        return true;
    } else {
        perror("invalid number");
        return false;
    }
}

void print_bytes(uint8_t *x, size_t len) {
    for (size_t i = 0; i < len; i++) {
        if (i % PRINT_BYTES_PER_LINE == 0 && i != 0) {
            printf("\n");
        }
        printf("%2.2x", x[i]);
    }
    putchar('\n');
}

void print_hash(uint8_t *x, size_t len) {
    hash_context ctx;
    hash_oneshot(&ctx, x, len);
    for (size_t i = 0; i < HASH_SIZE; i++) {
        printf("%2.2x", ctx.hash[i]);
    }
    putchar('\n');
}

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

void ls_range(f70s1arg_t *f70) {
    f70ret_t r;
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
        f70s1info_t *dir = f70->buf;
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

void ls_all(f70s1arg_t *f70) {
    f70ret_t r;
    while (true) {
        kos_fuse_lfn(f70, &r);
        printf("status = %d, count = %d\n", r.status, r.count);
        if (r.status != F70_SUCCESS && r.status != F70_END_OF_FILE) {
            abort();
        }
        f70s1info_t *dir = f70->buf;
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
    f70s1info_t *dir = (f70s1info_t*)malloc(sizeof(f70s1info_t) + sizeof(bdfe_t) * MAX_DIRENTS_TO_READ);
    f70s1arg_t f70 = {1, 0, CP866, MAX_DIRENTS_TO_READ, dir, 0, argv[1]};
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
    f70s5arg_t f70 = {.sf = 5, .flags = 0, .zero = 0};
    f70ret_t r;
    bdfe_t file;
    f70.buf = &file;
    f70.path = argv[1];
    kos_fuse_lfn(&f70, &r);
    printf("attr: 0x%2.2x\n", file.attr);
    printf("size: %llu\n", file.size);
    return;
}

/*
void read_range(struct f70s0arg *f70) {
    f70ret r;
    hash_context ctx;
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
        print_bytes(f70->buf, r.count);
        hash_oneshot(&ctx, f70->buf, r.count);
        print_hash(ctx.hash, HASH_SIZE);
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
        if () {
            print_bytes(f70->buf, r.count);
        }
        if () {
            hash_context ctx;
            hash_oneshot(&ctx, f70->buf, r.count);
            print_hash(ctx.hash, HASH_SIZE);
        }
        if (r.status == F70_END_OF_FILE) {
            break;
        }
    }
}
*/

void kofu_read(int argc, const char **argv) {
    (void)argc;
    f70s0arg_t f70 = {.sf = 0, .zero = 0};
    f70ret_t r;
    bool dump_bytes = false, dump_hash = false;
    if (argc < 4) {
        printf("usage: %s <offset> <length> [-b] [-h]\n", argv[0]);
        return;
    }
    int opt = 1;
    f70.path = argv[opt++];
    if ((opt >= argc) || !parse_uint64(argv[opt++], &f70.offset))
        return;
    if ((opt >= argc) || !parse_uint32(argv[opt++], &f70.count))
        return;
    for (; opt < argc; opt++) {
        if (!strcmp(argv[opt], "-b")) {
            dump_bytes = true;
        } else if (!strcmp(argv[opt], "-h")) {
            dump_hash = true;
        } else {
            printf("invalid option: '%s'\n", argv[opt]);
            return;
        }
    }
    f70.buf = (uint8_t*)malloc(f70.count);

    kos_fuse_lfn(&f70, &r);
    assert((r.status == F70_SUCCESS && r.count == f70.count) ||
           (r.status == F70_END_OF_FILE && r.count < f70.count));

    if (dump_bytes)
        print_bytes(f70.buf, r.count);
    if (dump_hash)
        print_hash(f70.buf, r.count);

    free(f70.buf);
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

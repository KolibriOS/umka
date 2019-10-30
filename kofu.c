#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <inttypes.h>
#include <limits.h>
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

#define PATH_MAX 4096
#define FGETS_BUF_LEN 4096
#define MAX_COMMAND_ARGS 42
#define PRINT_BYTES_PER_LINE 32
#define MAX_DIRENTS_TO_READ 100
#define MAX_BYTES_TO_READ (16*1024)

char cur_dir[PATH_MAX] = "/";
const char *last_dir = cur_dir;
bool cur_dir_changed = true;

char cmd_buf[FGETS_BUF_LEN];
int trace = false;

const char *f70_status_name[] = {
                                 "success",
                                 "disk_base",
                                 "unsupported_fs",
                                 "unknown_fs",
                                 "partition",
                                 "file_not_found",
                                 "end_of_file",
                                 "memory_pointer",
                                 "disk_full",
                                 "fs_fail",
                                 "access_denied",
                                 "device",
                                 "out_of_memory"
                                };

const char *get_f70_status_name(f70status s) {
    switch (s) {
    case F70_ERROR_SUCCESS:
//        return "";
    case F70_ERROR_DISK_BASE:
    case F70_ERROR_UNSUPPORTED_FS:
    case F70_ERROR_UNKNOWN_FS:
    case F70_ERROR_PARTITION:
    case F70_ERROR_FILE_NOT_FOUND:
    case F70_ERROR_END_OF_FILE:
    case F70_ERROR_MEMORY_POINTER:
    case F70_ERROR_DISK_FULL:
    case F70_ERROR_FS_FAIL:
    case F70_ERROR_ACCESS_DENIED:
    case F70_ERROR_DEVICE:
    case F70_ERROR_OUT_OF_MEMORY:
        return f70_status_name[s];
    default:
        return "unknown";
    }
}

void print_f70_status(f70ret_t *r, int use_ebx) {
    printf("status = %d %s", r->status, get_f70_status_name(r->status));
    if (use_ebx && (r->status == F70_ERROR_SUCCESS || r->status == F70_ERROR_END_OF_FILE))
        printf(", count = %d", r->count);
    putchar('\n');
}

bool parse_uintmax(const char *str, uintmax_t *res) {
    char *endptr;
    *res = strtoumax(str, &endptr, 0);
    bool ok = (str != endptr) && (*endptr == '\0');
    return ok;
}

bool parse_uint32(const char *str, uint32_t *res) {
    uintmax_t x;
    if (parse_uintmax(str, &x) && x <= UINT32_MAX) {
        *res = (uint32_t)x;
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

const char *get_last_dir(const char *path) {
    const char *last = strrchr(path, '/');
    if (!last) {
        last = path;
    } else if (last != path || last[1] != '\0') {
        last++;
    }
    return last;
}

void prompt() {
    if (cur_dir_changed) {
        kos_getcwd(cur_dir, PATH_MAX);
        last_dir = get_last_dir(cur_dir);
        cur_dir_changed = false;
    }
    printf("%s> ", last_dir);
    fflush(stdout);
}

int next_line(FILE *file, int is_tty) {
    if (is_tty) {
        prompt();
    }
    return fgets(cmd_buf, FGETS_BUF_LEN, file) != NULL;
}

int split_args(char *s, const char **argv) {
    int argc = -1;
    for (; (argv[++argc] = strtok(s, " \t\n")) != NULL; s = NULL);
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

void kofu_pwd(int argc, const char **argv) {
    (void)argc;
    (void)argv;
    bool quoted = false;
    const char *quote = quoted ? "'" : "";
    kos_getcwd(cur_dir, PATH_MAX);
    printf("%s%s%s\n", quote, cur_dir, quote);
}

void kofu_cd(int argc, const char **argv) {
    (void)argc;
    kos_cd(argv[1]);
    cur_dir_changed = true;
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
        kos_lfn(f70, &r);
        f70->offset += f70->size;
        print_f70_status(&r, 1);
        f70s1info_t *dir = f70->buf;
        assert(r.count <= f70->size);
        assert(dir->cnt == r.count);
        assert((r.status == F70_ERROR_SUCCESS && r.count == f70->size) ||
               (r.status == F70_ERROR_END_OF_FILE && r.count < f70->size)
              );
        for (size_t i = 0; i < dir->cnt; i++) {
            printf("%s\n", dir->bdfes[i].name);
        }
        if (r.status == F70_ERROR_END_OF_FILE) {
            break;
        }
    }
}

void ls_all(f70s1arg_t *f70) {
    f70ret_t r;
    while (true) {
        kos_lfn(f70, &r);
        print_f70_status(&r, 1);
        if (r.status != F70_ERROR_SUCCESS && r.status != F70_ERROR_END_OF_FILE) {
            abort();
        }
        f70s1info_t *dir = f70->buf;
        f70->offset += dir->cnt;
        assert(r.count <= f70->size);
        assert(dir->cnt == r.count);
        assert((r.status == F70_ERROR_SUCCESS && r.count == f70->size) ||
               (r.status == F70_ERROR_END_OF_FILE && r.count < f70->size)
              );
        for (size_t i = 0; i < dir->cnt; i++) {
            printf("%s\n", dir->bdfes[i].name);
        }
        if (r.status == F70_ERROR_END_OF_FILE) {
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
    kos_lfn(&f70, &r);
    print_f70_status(&r, 0);
    printf("attr: 0x%2.2x\n", file.attr);
    printf("size: %llu\n", file.size);
    return;
}

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

    kos_lfn(&f70, &r);

    assert(r.count <= f70.count);
    assert((r.count == f70.count && r.status == F70_ERROR_SUCCESS) ||
           (r.count < f70.count && r.status == F70_ERROR_END_OF_FILE)
          );

    print_f70_status(&r, 1);
    if (dump_bytes)
        print_bytes(f70.buf, r.count);
    if (dump_hash)
        print_hash(f70.buf, r.count);

    free(f70.buf);
    return;
}

typedef struct {
    char *name;
    void (*func) (int, const char **);
} func_table_t;

func_table_t funcs[] = {
                              { "disk_add", kofu_disk_add },
                              { "disk_del", kofu_disk_del },
                              { "ls",   kofu_ls   },
                              { "stat", kofu_stat },
                              { "read", kofu_read },
                              { "pwd",  kofu_pwd },
                              { "cd",   kofu_cd },
                              { NULL,   NULL      },
                            };

void usage() {
    printf("usage: kofu [test_file.t]\n");
}

void *run_test(const char *infile_name) {
    FILE *infile, *outfile;

    if (!infile_name) {
        infile = stdin;
        outfile = stdout;
    } else {
        char outfile_name[PATH_MAX];
        strncpy(outfile_name, infile_name, PATH_MAX-2);    // ".t" is shorter that ".out"
        char *last_dot = strrchr(outfile_name, '.');
        if (!last_dot) {
            printf("test file must have '.t' suffix\n");
            usage();
            return NULL;
        }
        strcpy(last_dot, ".out");
        infile = fopen(infile_name, "r");
        outfile = fopen(outfile_name, "w");
        if (!infile || !outfile) {
            printf("can't open in/out files\n");
            return NULL;
        }
    }

    int is_tty = isatty(fileno(infile));
    const char **cargv = (const char**)malloc(sizeof(const char*) * (MAX_COMMAND_ARGS + 1));
    while(next_line(infile, is_tty)) {
        if (cmd_buf[0] == '#' || cmd_buf[0] == '\n') {
            printf("%s", cmd_buf);
            continue;
        }
        if (cmd_buf[0] == 'X') break;
        if (!is_tty) {
            prompt();
            printf("%s", cmd_buf);
            fflush(outfile);
        }
        int cargc = split_args(cmd_buf, cargv);
        func_table_t *ft;
        for (ft = funcs; ft->name != NULL; ft++) {
            if (!strcmp(cargv[0], ft->name)) {
                break;
            }
        }
        if (ft->name) {
            ft->func(cargc, cargv);
        } else {
            printf("unknown command: %s\n", cargv[0]);
        }
    }
    free(cargv);

    return NULL;
}

int main(int argc, char **argv) {
    if (trace)
        trace_begin();
    kos_init();

    switch (argc) {
    case 1:
        run_test(NULL);
        break;
    case 2: {
        run_test(argv[1]);
        break;
    }
    default:
        usage();
    }

    if (trace)
        trace_end();

    return 0;
}

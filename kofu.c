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
#include <time.h>
#include "kolibri.h"
#include "trace.h"

#define PATH_MAX 4096
#define FGETS_BUF_LEN 4096
#define MAX_COMMAND_ARGS 42
#define PRINT_BYTES_PER_LINE 32
#define MAX_DIRENTS_TO_READ 100
#define MAX_BYTES_TO_READ (16*1024)

#define DEFAULT_PATH_ENCODING UTF8

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

const char *get_f70_status_name(f70status_t s) {
    switch (s) {
    case ERROR_SUCCESS:
//        return "";
    case ERROR_DISK_BASE:
    case ERROR_UNSUPPORTED_FS:
    case ERROR_UNKNOWN_FS:
    case ERROR_PARTITION:
    case ERROR_FILE_NOT_FOUND:
    case ERROR_END_OF_FILE:
    case ERROR_MEMORY_POINTER:
    case ERROR_DISK_FULL:
    case ERROR_FS_FAIL:
    case ERROR_ACCESS_DENIED:
    case ERROR_DEVICE:
    case ERROR_OUT_OF_MEMORY:
        return f70_status_name[s];
    default:
        return "unknown";
    }
}

void convert_f70_file_attr(uint32_t attr, char s[KF_ATTR_CNT+1]) {
    s[0] = (attr & KF_READONLY) ? 'r' : '-';
    s[1] = (attr & KF_HIDDEN)   ? 'h' : '-';
    s[2] = (attr & KF_SYSTEM)   ? 's' : '-';
    s[3] = (attr & KF_LABEL)    ? 'l' : '-';
    s[4] = (attr & KF_FOLDER)   ? 'f' : '-';
    s[5] = '\0';
}

void print_f70_status(f7080ret_t *r, int use_ebx) {
    printf("status = %d %s", r->status, get_f70_status_name(r->status));
    if (use_ebx && (r->status == ERROR_SUCCESS || r->status == ERROR_END_OF_FILE))
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

void ls_range(f7080s1arg_t *fX0, f70or80_t f70or80) {
    f7080ret_t r;
    size_t bdfe_len = (fX0->encoding == CP866) ? BDFE_LEN_CP866 : BDFE_LEN_UNICODE;
    uint32_t requested = fX0->size;
    if (fX0->size > MAX_DIRENTS_TO_READ) {
        fX0->size = MAX_DIRENTS_TO_READ;
    }
    for (; requested; requested -= fX0->size) {
        if (fX0->size > requested) {
            fX0->size = requested;
        }
        kos_lfn(fX0, &r, f70or80);
        fX0->offset += fX0->size;
        print_f70_status(&r, 1);
        f7080s1info_t *dir = fX0->buf;
        int ok = (r.count <= fX0->size);
        ok &= (dir->cnt == r.count);
        ok &= (r.status == ERROR_SUCCESS && r.count == fX0->size)
              || (r.status == ERROR_END_OF_FILE && r.count < fX0->size);
        assert(ok);
        if (!ok)
            break;
        bdfe_t *bdfe = dir->bdfes;
        for (size_t i = 0; i < dir->cnt; i++) {
            char fattr[KF_ATTR_CNT+1];
            convert_f70_file_attr(bdfe->attr, fattr);
            printf("%s %s\n", fattr, bdfe->name);
            bdfe = (bdfe_t*)((uintptr_t)bdfe + bdfe_len);
        }
        if (r.status == ERROR_END_OF_FILE) {
            break;
        }
    }
}

void ls_all(f7080s1arg_t *fX0, f70or80_t f70or80) {
    f7080ret_t r;
    size_t bdfe_len = (fX0->encoding == CP866) ? BDFE_LEN_CP866 : BDFE_LEN_UNICODE;
    while (true) {
        kos_lfn(fX0, &r, f70or80);
        print_f70_status(&r, 1);
        assert((r.status == ERROR_SUCCESS && r.count == fX0->size)
              || (r.status == ERROR_END_OF_FILE && r.count < fX0->size));
        f7080s1info_t *dir = fX0->buf;
        fX0->offset += dir->cnt;
        int ok = (r.count <= fX0->size);
        ok &= (dir->cnt == r.count);
        ok &= (r.status == ERROR_SUCCESS && r.count == fX0->size)
              || (r.status == ERROR_END_OF_FILE && r.count < fX0->size);
        assert(ok);
        if (!ok)
            break;
        printf("total = %"PRIi32"\n", dir->total_cnt);
        bdfe_t *bdfe = dir->bdfes;
        for (size_t i = 0; i < dir->cnt; i++) {
            char fattr[KF_ATTR_CNT+1];
            convert_f70_file_attr(bdfe->attr, fattr);
            printf("%s %s\n", fattr, bdfe->name);
            bdfe = (bdfe_t*)((uintptr_t)bdfe + bdfe_len);
        }
        if (r.status == ERROR_END_OF_FILE) {
            break;
        }
    }
}

void kofu_ls(int argc, const char **argv, f70or80_t f70or80) {
    (void)argc;
    uint32_t encoding = UTF8;
    size_t bdfe_len = (encoding == CP866) ? BDFE_LEN_CP866 : BDFE_LEN_UNICODE;
    f7080s1info_t *dir = (f7080s1info_t*)malloc(sizeof(f7080s1info_t) + bdfe_len * MAX_DIRENTS_TO_READ);
    f7080s1arg_t fX0 = {.sf = 1, .offset = 0, .encoding = encoding, .size = MAX_DIRENTS_TO_READ, .buf = dir};
    if (f70or80 == F70) {
        fX0.u.f70.zero = 0;
        fX0.u.f70.path = argv[1];
    } else {
        fX0.u.f80.path_encoding = DEFAULT_PATH_ENCODING;
        fX0.u.f80.path = argv[1];
    }
    if (argv[2]) {
        sscanf(argv[2], "%"SCNu32, &fX0.size);
        if (argv[3]) {
            sscanf(argv[3], "%"SCNu32, &fX0.offset);
        }
        ls_range(&fX0, f70or80);
    } else {
        ls_all(&fX0, f70or80);
    }
    free(dir);
    return;
}

void kofu_ls70(int argc, const char **argv) {
    kofu_ls(argc, argv, F70);
}

void kofu_ls80(int argc, const char **argv) {
    kofu_ls(argc, argv, F80);
}

void kofu_stat(int argc, const char **argv, f70or80_t f70or80) {
    (void)argc;
    f7080s5arg_t fX0 = {.sf = 5, .flags = 0};
    f7080ret_t r;
    bdfe_t file;
    fX0.buf = &file;
    if (f70or80 == F70) {
        fX0.u.f70.zero = 0;
        fX0.u.f70.path = argv[1];
    } else {
        fX0.u.f80.path_encoding = DEFAULT_PATH_ENCODING;
        fX0.u.f80.path = argv[1];
    }
    kos_lfn(&fX0, &r, f70or80);
    print_f70_status(&r, 0);
    if (r.status != ERROR_SUCCESS)
        return;
    char fattr[KF_ATTR_CNT+1];
    convert_f70_file_attr(file.attr, fattr);
    printf("attr: %s\n", fattr);
    if ((file.attr & KF_FOLDER) == 0) {   // don't show size for dirs
        printf("size: %llu\n", file.size);
    }

#if PRINT_DATE_TIME == 1
    time_t time;
    struct tm *t;
    time = kos_time_to_epoch(&file.ctime);
    t = localtime(&time);
    printf("ctime: %4.4i.%2.2i.%2.2i %2.2i:%2.2i:%2.2i\n",
           t->tm_year + 1900, t->tm_mon + 1, t->tm_mday,
           t->tm_hour, t->tm_min, t->tm_sec);
    time = kos_time_to_epoch(&file.atime);
    t = localtime(&time);
    printf("atime: %4.4i.%2.2i.%2.2i %2.2i:%2.2i:%2.2i\n",
           t->tm_year + 1900, t->tm_mon + 1, t->tm_mday,
           t->tm_hour, t->tm_min, t->tm_sec);
    time = kos_time_to_epoch(&file.mtime);
    t = localtime(&time);
    printf("mtime: %4.4i.%2.2i.%2.2i %2.2i:%2.2i:%2.2i\n",
           t->tm_year + 1900, t->tm_mon + 1, t->tm_mday,
           t->tm_hour, t->tm_min, t->tm_sec);
#endif
    return;
}

void kofu_stat70(int argc, const char **argv) {
    kofu_stat(argc, argv, F70);
}

void kofu_stat80(int argc, const char **argv) {
    kofu_stat(argc, argv, F80);
}

void kofu_read(int argc, const char **argv, f70or80_t f70or80) {
    (void)argc;
    f7080s0arg_t fX0 = {.sf = 0};
    f7080ret_t r;
    bool dump_bytes = false, dump_hash = false;
    if (argc < 4) {
        printf("usage: %s <offset> <length> [-b] [-h] [-e cp866|utf8|utf16]\n", argv[0]);
        return;
    }
    int opt = 1;
    if (f70or80 == F70) {
        fX0.u.f70.zero = 0;
        fX0.u.f70.path = argv[opt++];
    } else {
        fX0.u.f80.path_encoding = DEFAULT_PATH_ENCODING;
        fX0.u.f80.path = argv[opt++];
    }
    if ((opt >= argc) || !parse_uint64(argv[opt++], &fX0.offset))
        return;
    if ((opt >= argc) || !parse_uint32(argv[opt++], &fX0.count))
        return;
    for (; opt < argc; opt++) {
        if (!strcmp(argv[opt], "-b")) {
            dump_bytes = true;
        } else if (!strcmp(argv[opt], "-h")) {
            dump_hash = true;
        } else if (!strcmp(argv[opt], "-e")) {
            if (f70or80 == F70) {
                printf("f70 doesn't accept encoding parameter, use f80\n");
                return;
            }
        } else {
            printf("invalid option: '%s'\n", argv[opt]);
            return;
        }
    }
    fX0.buf = (uint8_t*)malloc(fX0.count);

    kos_lfn(&fX0, &r, f70or80);

    print_f70_status(&r, 1);
    if (r.status == ERROR_SUCCESS || r.status == ERROR_END_OF_FILE) {
        if (dump_bytes)
            print_bytes(fX0.buf, r.count);
        if (dump_hash)
            print_hash(fX0.buf, r.count);
    }

    free(fX0.buf);
    return;
}

void kofu_read70(int argc, const char **argv) {
    kofu_read(argc, argv, F70);
}

void kofu_read80(int argc, const char **argv) {
    kofu_read(argc, argv, F80);
}

typedef struct {
    char *name;
    void (*func) (int, const char **);
} func_table_t;

func_table_t funcs[] = {
                              { "disk_add", kofu_disk_add },
                              { "disk_del", kofu_disk_del },
                              { "ls70",     kofu_ls70 },
                              { "ls80",     kofu_ls80 },
                              { "stat70",   kofu_stat70 },
                              { "stat80",   kofu_stat80 },
                              { "read70",   kofu_read70 },
                              { "read80",   kofu_read80 },
                              { "pwd",      kofu_pwd },
                              { "cd",       kofu_cd },
                              { NULL,       NULL },
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

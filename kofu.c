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
#include "kolibri.h"

#define FGETS_BUF_LEN 4096
#define MAX_COMMAND_ARGS 42
#define DIRENTS_TO_READ 100

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
    int status = kos_fuse_lfn(f70);
//    printf("status = %d\n", status);
    if (status == F70_SUCCESS || status == F70_END_OF_FILE) {
        struct f70s1ret *dir = f70->buf;
        for (size_t i = 0; i < dir->cnt; i++) {
            printf("%s\n", dir->bdfes[i].name);
        }
    }
}

void ls_all(struct f70s1arg *f70) {
    while (true) {
        int status = kos_fuse_lfn(f70);
//        printf("status = %d\n", status);
        if (status != F70_SUCCESS && status != F70_END_OF_FILE) {
            abort();
        }
        struct f70s1ret *dir = f70->buf;
        f70->offset += dir->cnt;

        for (size_t i = 0; i < dir->cnt; i++) {
            printf("%s\n", dir->bdfes[i].name);
        }

        if (status == F70_END_OF_FILE) {
            break;
        }
    }
}

void kofu_ls(char **arg) {
    struct f70s1ret *dir = (struct f70s1ret*)malloc(sizeof(struct f70s1ret) + sizeof(struct bdfe) * DIRENTS_TO_READ);
    struct f70s1arg f70 = {1, 0, CP866, DIRENTS_TO_READ, dir, 0, arg[1]};
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
    struct bdfe file;
    struct f70s5arg f70 = {5, 0, 0, 0, &file, 0, arg[1]};
    kos_fuse_lfn(&f70);
    printf("attr: 0x%2.2x\n", file.attr);
    printf("size: %llu\n", file.size);
    return;
}

void kofu_read(char **arg) {
    size_t size;
    off_t offset;
    sscanf(arg[2], "%zu", &size);
    sscanf(arg[3], "%llu", &offset);
    uint8_t *buf = (uint8_t*)malloc(size);
    struct f70s5arg f70 = {0, offset, offset >> 32, size, buf, 0, arg[1]};
    kos_fuse_lfn(&f70);
    for (size_t i = 0; i < size; i++) {
        if (i % 32 == 0 && i != 0) {
            printf("\n");
        }
        printf("%2.2x", buf[i]);
    }
    printf("\n");
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
    return 0;
}

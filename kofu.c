#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "kocdecl.h"

#define FGETS_BUF_LEN 4096
#define MAX_COMMAND_ARGS 42

char cmd_buf[FGETS_BUF_LEN];
bool is_tty;
int cmd_num = 0;

void prompt() {
    fprintf(stderr, "#%d> ", cmd_num);
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

void kofu_ls(char **arg) {
    void *header = kos_fuse_readdir(arg[1], 0);
    uint32_t file_cnt = ((uint32_t*)header)[1];
    printf("file_cnt: %u\n", file_cnt);
    struct bdfe *kf = header + 0x20;
    for (; file_cnt > 0; file_cnt--) {
        printf("%s\n", kf->name);
        kf++;
    }
    return;
}

void kofu_stat(char **arg) {
    struct bdfe *kf = kos_fuse_getattr(arg[1]);
    printf("attr: 0x%2.2x\n", kf->attr);
    printf("size: %llu\n", kf->size);
    return;
}

void kofu_read(char **arg) {
    size_t size;
    size_t offset;
    sscanf(arg[2], "%zu", &size);
    sscanf(arg[3], "%zu", &offset);
    char *buf = (char*)malloc(size + 4096);
    kos_fuse_read(arg[1], buf, size, offset);
    for (size_t i = 0; i < size; i++) {
        if (i % 32 == 0 && i != 0) {
            printf("\n");
        }
        printf("%2.2x", buf[i]);
    }
    printf("\n");
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
    if (!kos_fuse_init(fd)) {
        exit(1);
    }

//msg_few_args             db 'usage: xfskos image [offset]',0x0a
//msg_file_not_found       db 'file not found: '
//msg_unknown_command      db 'unknown command',0x0a
//msg_not_xfs_partition    db 'not xfs partition',0x0a
    while(next_line()) {
        if (!is_tty) {
            prompt();
            fprintf(stderr, "%s", cmd_buf);
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

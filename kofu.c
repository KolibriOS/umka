#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "kocdecl.h"

void prompt() {
    printf("> ");
    fflush(stdout);
}

int main(int argc, char **argv) {
    char cmd_buf[4096];
    int fd = open(argv[1], O_RDONLY);
    kos_fuse_init(fd);
    
//msg_few_args             db 'usage: xfskos image [offset]',0x0a
//msg_file_not_found       db 'file not found: '
//msg_unknown_command      db 'unknown command',0x0a
//msg_not_xfs_partition    db 'not xfs partition',0x0a
    while(true) {
        prompt();
        fgets(cmd_buf, 4095, stdin);
        int len = strlen(cmd_buf);
        cmd_buf[len-1] = '\0';
//        printf("'%s'\n", cmd_buf);
        if (!strncmp(cmd_buf, "ls ", 3)) {
            void *header = kos_fuse_readdir(cmd_buf + 4, 0);
            uint32_t file_cnt = ((uint32_t*)header)[1];
            printf("file_cnt: %u\n", file_cnt);
            struct bdfe *kf = header + 0x20;
            for (; file_cnt > 0; file_cnt--) {
                printf("%s\n", kf->name);
                kf++;
            }
        } else if (!strncmp(cmd_buf, "stat ", 5)) {
            struct bdfe *kf = kos_fuse_getattr(cmd_buf + 5);
            printf("attr: 0x%2.2x\n", kf->attr);
            printf("size: %llu\n", kf->size);
        } else {
            printf("unknown command: %s\n", cmd_buf);
        }
    }

    return 0;
}

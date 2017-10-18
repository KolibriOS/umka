#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
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
        scanf("%s", cmd_buf);
        char *bdfe = kos_fuse_readdir("", 0);
        uint32_t file_cnt = *(uint32_t*)(bdfe + 4);
        bdfe += 0x20;
        for (; file_cnt > 0; file_cnt--) {
            printf("%s\n", bdfe + 0x28);
            bdfe += 304;
        }
    }

    return 0;
}

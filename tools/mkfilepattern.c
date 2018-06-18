#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>

#define BUF_LEN 0x10

int main(int argc, char *argv[])
{
    uint8_t buf[BUF_LEN + 7];
    unsigned len;
    char *path;
    if (argc != 3) {
        fprintf(stderr, "mkfilepattern filename size\n");
        exit(1);
    } else {
        path = argv[1];
        sscanf(argv[2], "%u", &len);
    }

    int fd = open(path, O_CREAT | O_LARGEFILE | O_WRONLY, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
    if (fd == -1) {
        fprintf(stderr, "Can't open %s: %s\n", path, strerror(errno));
        exit(1);
    }

    for (unsigned pos = 0, count = BUF_LEN; pos < len; pos += count) {
        if (count > len - pos) {
            count = len - pos;
        }

        off64_t off = 0;
        while (off < count) {
            if (pos + off < 0x100) {
                *(uint8_t*)(buf + off) = (uint8_t)(pos + off);
                off += 1;
            } else if (pos + off < 0x10000) {
                *(uint16_t*)(buf + off) = (uint16_t)(pos + off);
                off += 2;
            } else if (pos + off < 0x100000000l) {
                *(uint32_t*)(buf + off) = (uint32_t)(pos + off);
                off += 4;
            } else {
                *(uint64_t*)(buf + off) = (uint64_t)(pos + off);
                off += 8;
            }
        }

        write(fd, buf, count);
    }

    close(fd);

    return 0;
}

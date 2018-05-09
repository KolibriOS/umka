#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include <string.h>


int main(int argc, char *argv[])
{
    unsigned long   begin, end, current;
    char dirname[256] = "dir0123456789_blahblahblahblahblahblahblahblahblahblahblahblahblahblahblahblahblahblahblahblahblahblahblahblah\0";

    if(argc != 3) {
        fprintf(stderr, "%s num_begin num_end\n", argv[0]);
        exit(1);
    }

    sscanf(argv[1], "%lu", &begin);
    sscanf(argv[2], "%lu", &end);

    for(current=begin; current<=end; current++) {
        sprintf(dirname + 3, "%10.10lu", current);
        dirname[13] = '_';
        if(mkdir(dirname, 0755)) {
            fprintf(stderr, "error: %s\n", strerror(errno));
            exit(1);
        }
    }

    return 0;
}

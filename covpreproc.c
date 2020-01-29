#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>

#define MAX_COVERED_CODE_SIZE (128*1024)

typedef struct {
    uint64_t to_cnt, from_cnt;
} branch;

branch branches[MAX_COVERED_CODE_SIZE];

uint32_t coverage_offset, coverage_begin, coverage_end;

void read_coverage_file(const char *fname) {
    FILE *f = fopen(fname, "r+");
    fseeko(f, 0, SEEK_END);
    off_t fsize = ftello(f);
    fseeko(f, 0, SEEK_SET);
    size_t branch_cnt = fsize/(2*sizeof(uint32_t));
    for (size_t i = 0; i < branch_cnt; i++) {
        uint32_t from, to;
        fread(&from, sizeof(uint32_t), 1, f);
        fread(&to, sizeof(uint32_t), 1, f);
        if (from >= coverage_begin && from < coverage_end /*&& to < 0x80000000u*/) {
            from = from - coverage_begin + coverage_offset;
            branches[from].from_cnt++;
        }
        if (to >= coverage_begin && to < coverage_end /*&& from < 0x80000000u*/) {
            to = to - coverage_begin + coverage_offset;
            branches[to].to_cnt++;
        }
    }
    fclose(f);
}

size_t count_line_bytes(const char *s) {
    size_t cnt = 0;
    for (size_t i = 10; i <= 58; i += 3) {
        if (s[i] == ' ') break;
        cnt++;
    }
    return cnt;
}

size_t count_block_bytes(FILE *f) {
    char tmp[1024];
    size_t cnt = 0;
    if (fgets(tmp, 1024, f) && strspn(tmp, "0123456789ABCDEF") == 8) {
        cnt = count_line_bytes(tmp);
        while (fgets(tmp, 1024, f) && tmp[0] == ' ' && tmp[10] != ' ') {
            cnt += count_line_bytes(tmp);
        }
    }
    return cnt;
}

int main(int argc, char **argv) {
    if (argc < 6) {
        fprintf(stderr, "usage: covpreproc <listing file> <coverage_begin offset> <coverage_begin address> <coverage_end address> <coverage files ...>\n");
        exit(1);
    }
    sscanf(argv[2], "%" SCNx32, &coverage_offset);
    sscanf(argv[3], "%" SCNx32, &coverage_begin);
    sscanf(argv[4], "%" SCNx32, &coverage_end);

    for (int i = 5; i < argc; i++) {
        read_coverage_file(argv[i]);
    }

    FILE *f = fopen(argv[1], "r");
    char tmp[1024];
    uint64_t cur = 0;
    while (1) {
        off_t fpos_before = ftello(f);
        if (!fgets(tmp, 1024, f)) {
            break;
        }
        off_t fpos_after = ftello(f);
        if (strspn(tmp, "0123456789ABCDEF") == 8) {
            fseeko(f, fpos_before, SEEK_SET);
            size_t inst_len = count_block_bytes(f);
            fseeko(f, fpos_after, SEEK_SET);
            unsigned long pos = strtoul(tmp, NULL, 16);
            size_t total_to = 0, total_from = 0;
            for (size_t i = 0; i < inst_len; i++) {
                total_to += branches[pos + i].to_cnt;
                total_from += branches[pos + i].from_cnt;
            }
            cur += total_to;
            if (cur) {
                putchar(' ');
            } else {
                putchar('-');
            }
            if (total_from || total_to || cur) {
                printf("%8" PRIu64 " %8" PRIu64 " %8" PRIu64, total_from, total_to, cur);
            } else {
                printf("       -        -        -");
            }
            cur -= total_from;
        } else {
            printf("                           ");
        }
        printf(" : %s", tmp + 64);
    }
    fclose(f);

    return 0;
}

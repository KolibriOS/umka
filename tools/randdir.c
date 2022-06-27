/*
    SPDX-License-Identifier: GPL-2.0-or-later

    UMKa - User-Mode KolibriOS developer tools

    Copyright (C) 2019-2020  Ivan Baravy <dunkaist@gmail.com>
    Copyright (C) 2021  Magomed Kostoev <mkostoevr@yandex.ru>
*/

#include <stdio.h>
#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>

#define PRINTABLES "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
char *printables = PRINTABLES;
size_t printables_cnt = sizeof(PRINTABLES);

unsigned uint_hash(unsigned x) {
    x = ((x >> 16) ^ x) * 0x45d9f3b;
    x = ((x >> 16) ^ x) * 0x45d9f3b;
    x = (x >> 16) ^ x;
    return x;
}

void create_directory(int root_dirfd, char *path) {
    if(mkdirat(root_dirfd, path, 0755)) {
        fprintf(stderr, "Can't mkdir %s: %s\n", path, strerror(errno));
        exit(1);
    }
}

void create_file_impl(int rootdirfd, char *path, size_t size, char *contents) {
    int fd = openat(rootdirfd, path, O_CREAT | O_WRONLY, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
    if (fd == -1) {
        fprintf(stderr, "Can't open %s: %s\n", path, strerror(errno));
        exit(1);
    }
    write(fd, contents, size);
    close(fd);
}

void create_file(int rootdirfd, char *path, size_t max_size, unsigned seed) {
    size_t path_len = strlen(path);
    char *header = "<FILE>";
    char *footer = "</FILE>";
    size_t header_size = strlen(header);
    size_t footer_size = strlen(footer);
    size_t data_size = seed % max_size;
    if (data_size + header_size + footer_size > max_size) {
        data_size = max_size - header_size - footer_size;
    }
    size_t file_size = data_size + header_size + footer_size;
    assert(file_size <= max_size);
    char *contents = calloc(1, file_size);
    if (contents == NULL) {
        fprintf(stderr, "Can't allocate memory for the file of size %lu\n", file_size);
        exit(1);
    }
    // First is header
    memcpy(contents, header, header_size);
    // Then is data
    for (unsigned i = header_size; i < header_size + data_size; i++) {
        contents[i] = (char)uint_hash(i) ^ path[i % path_len] ^ (char)seed;
    }
    // The last is footer
    memcpy(contents + header_size + data_size, footer, footer_size);
    create_file_impl(rootdirfd, path, file_size, contents);
}

void path_go_back(char *path) {
    char *last_separator = strrchr(path, '/');
    if (last_separator) {
        *last_separator = '\0';
    }
}

void path_append_impl(char *path, unsigned path_max, unsigned name_max, unsigned seed) {
    size_t path_len = strlen(path);
    unsigned name_len = seed % name_max;

    if (name_len == 0) {
        name_len = 1;
    }

    if (path_len + 1 + name_len >= path_max) {
        return;
    }

    strcat(path, "/");
    char name_buf[name_len + 1];
    for (unsigned i = 0; i < name_len; i++) {
        size_t rand_idx = seed ^ (139 * i);
        name_buf[i] = printables[rand_idx % printables_cnt];
    }
    name_buf[name_len] = '\0';
    strcat(path, name_buf);
}

int file_exists(int rootdirfd, char *path) {
    return !faccessat(rootdirfd, path, W_OK, 0);
}

void path_append(int rootdirfd, char *path, unsigned path_max, unsigned name_max, unsigned seed) {
    do {
        path_append_impl(path, path_max, name_max, seed);
        if (file_exists(rootdirfd, path)) {
            // Generate new feed in case if the current one generated existing filename
            seed = uint_hash(seed);
            path_go_back(path);
            continue;
        }
        break;
    } while (1);
}

int in_range(unsigned v, unsigned begin, unsigned end) {
    return v >= begin && v <= end;
}

int main(int argc, char *argv[])
{
    unsigned random_things_count, name_max, path_max, file_size_max;
    char *path;
    if (argc == 6) {
        path = argv[1];
        sscanf(argv[2], "%u", &random_things_count);
        sscanf(argv[3], "%u", &name_max);
        sscanf(argv[4], "%u", &path_max);
        sscanf(argv[5], "%u", &file_size_max);
    } else {
        fprintf(stderr, "randdir <directory> <random_things_count> <name_max> <path_max> <file_size_max>\n"
                        "    directory           - the folder to create random stuff in\n"
                        "    random_things_count - count of things (folders and files) to generate\n"
                        "    name_max            - max length of a file or folder name\n"
                        "    path_max            - max length of a path relative to the root folder\n"
                        "    file_size_max       - max size of a generated file");
        exit(1);
    }

    int rootdirfd = open(path, O_DIRECTORY);
    if (rootdirfd == -1) {
        fprintf(stderr, "Can't open root folder (%s): %s\n", path, strerror(errno));
        exit(1);
    }

    char path_buf[path_max + 1];
    strcpy(path_buf, ".");
    for (unsigned i = 0; i < random_things_count; i++) {
        unsigned h = uint_hash(i);
        // Create a folder and get into it in 15% of cases
        if (in_range(h % 100, 85, 99)) {
            path_append(rootdirfd, path_buf, path_max, name_max, h);
            create_directory(rootdirfd, path_buf);
            continue;
        }
        // Get out of the folder in 15% of cases
        if (in_range(h % 100, 0, 14)) {
            path_go_back(path_buf);
        }
        // Create a file in 70% of cases
        path_append(rootdirfd, path_buf, path_max, name_max, h);
        create_file(rootdirfd, path_buf, file_size_max, h);
        path_go_back(path_buf);
    }

    return 0;
}

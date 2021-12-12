/*
    UMKa - User-Mode KolibriOS developer tools
    umka_gen_devices_dat - generate devices.dat file with IRQ information
    Copyright (C) 2021  Ivan Baravy <dunkaist@gmail.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#include <dirent.h>
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include "umka.h"
#include "util.h"
#include <pci.h>

#define UMKA_DEFAULT_DISPLAY_WIDTH 400
#define UMKA_DEFAULT_DISPLAY_HEIGHT 300

#define ACPI_CALL_DEV "/proc/acpi/call"
#define PCI_BASE_DIR "/sys/bus/pci/devices"
#define ACPI_BUF_SIZE 0x1000

#define STR2UINT32(s) ((s[3] << 24) | (s[2] << 16) | (s[1] << 8) | s[0])

char acpi_path_file[PATH_MAX];
char acpi_path[PATH_MAX];
char acpi_value[ACPI_BUF_SIZE];
amlctx_t *ctx;

size_t strcnt(const char *s, char c) {
    size_t count = 0;
    while (*s) {
        if (*s == c) {
            count += 1;
        }
        s++;
    }
    return count;
}

uint64_t acpi_get_next_number(char **str) {
    char *s = *str;
    while (*s == '[' || *s == ']' || *s == ' ' || *s == ',') {
        s++;
    }
    uint64_t result = strtoull(s, str, 16);
    return result;
}

acpi_node_t *acpi_get(const char *path) {
    FILE *f;
    f = fopen(ACPI_CALL_DEV, "w");
    if (!f) {
        perror("Can't open file");
        exit(1);
    }
    fwrite(path, strlen(path), 1, f);
    fclose(f);
    f = fopen(ACPI_CALL_DEV, "r");
    if (!f) {
        perror("Can't open file");
        exit(1);
    }
    size_t len = fread(acpi_value, 1, ACPI_BUF_SIZE, f);
    acpi_value[len-1] = '\0';
    fclose(f);
    acpi_node_t *n = NULL;
    char *str = acpi_value;
    if (acpi_value[0] == '0') {
        kos_node_integer_t *aint = (kos_node_integer_t*)kos_aml_constructor_integer();
        aint->value = strtoull(acpi_value, NULL, 16);
        n = (acpi_node_t*)aint;
    } else if (acpi_value[0] == '[') {
        size_t open_cnt = strcnt(acpi_value, '[');
        size_t close_cnt = strcnt(acpi_value, ']');
        if (open_cnt != close_cnt) {
            printf("acpi_call buffer is too short!\n");
            exit(1);
        }
        size_t el_cnt = open_cnt - 1;
        kos_node_package_t *apkg = (kos_node_package_t*)kos_aml_constructor_package(el_cnt);
        n = (acpi_node_t *)apkg;
        for (size_t i = 0; i < el_cnt; i++) {
            kos_node_package_t *p4 = (kos_node_package_t*)kos_aml_constructor_package(4);
            p4->list[0] = kos_aml_constructor_integer();
            ((kos_node_integer_t*)(p4->list[0]))->value = acpi_get_next_number(&str);
            p4->list[1] = kos_aml_constructor_integer();
            ((kos_node_integer_t*)(p4->list[1]))->value = acpi_get_next_number(&str);
            p4->list[2] = kos_aml_constructor_integer();
            ((kos_node_integer_t*)(p4->list[2]))->value = acpi_get_next_number(&str);
            p4->list[3] = kos_aml_constructor_integer();
            ((kos_node_integer_t*)(p4->list[3]))->value = acpi_get_next_number(&str);
            apkg->list[i] = (acpi_node_t*)p4;
        }
    }
    return n;
}

acpi_node_t *acpi_get_at(acpi_node_t *node, uint32_t name) {
    char path[PATH_MAX];
    path[PATH_MAX-1] = '\0';
    *(uint32_t*)(path + PATH_MAX - 5) = name;
    path[PATH_MAX-6] = '.';
    char *cur;
    for (cur = path + PATH_MAX - 6; node->parent; node = node->parent) {
        cur -= 5;
        cur[0] = '.';
        *(uint32_t*)(cur+1) = node->name;
    }
    cur[0] = '\\';
    acpi_node_t *n = acpi_get(cur);
    n->name = name;
    return n;
}

void acpi_process_dev(char *path) {
    size_t dir_name_len = strlen(path) - 1;
    acpi_node_t *parent = kos_acpi_root;
    acpi_node_t *new_parent;
    for (char *name = path + 1; name < path + dir_name_len; name += 5) {
        name[4] = '\0';
        if (!(new_parent = kos_acpi_lookup_node(parent, name))) {
            new_parent = kos_aml_alloc_node(KOS_ACPI_NODE_Device);
            new_parent->name = *(uint32_t*)name;
            kos_aml_attach(parent, new_parent);
            acpi_node_t *adr = acpi_get_at(new_parent, STR2UINT32("_ADR"));
            kos_aml_attach(new_parent, adr);
            if (name < path + dir_name_len - 5) {
                acpi_node_t *prt = acpi_get_at(new_parent, STR2UINT32("_PRT"));
                kos_aml_attach(new_parent, prt);
            }
        }
        parent = new_parent;
    }
}

int
main () {
    kos_boot.bpp = 32;
    kos_boot.x_res = UMKA_DEFAULT_DISPLAY_WIDTH;
    kos_boot.y_res = UMKA_DEFAULT_DISPLAY_HEIGHT;
    kos_boot.pitch = UMKA_DEFAULT_DISPLAY_WIDTH*4;  // 32bpp

    strcpy(pci_path, PCI_BASE_DIR);

    umka_init();
    kos_acpi_aml_init();
    ctx = kos_acpi_aml_new_thread();
    kos_acpi_dev_size = MAX_PCI_DEVICES*16;
    kos_acpi_dev_data = kos_kernel_alloc(kos_acpi_dev_size);
    kos_acpi_dev_next = kos_acpi_dev_data;

    DIR *pci_base_dir = opendir(PCI_BASE_DIR);
    if (!pci_base_dir) {
        perror("Can't open dir");
        exit(1);
    }
    struct dirent *d;
    while ((d = readdir(pci_base_dir))) {
        if (d->d_name[0] == '.') {
            continue;
        }
        sprintf(acpi_path_file, "%s/%s/%s", PCI_BASE_DIR, d->d_name,
                "firmware_node/path");
        FILE *f = fopen(acpi_path_file, "r");
        if (!f) {
            if (errno == ENOENT) {
                continue;
            } else {
                perror("Can't open file");
            }
        }
        
        size_t len = fread(acpi_path, 1, PATH_MAX, f);
        acpi_path[len-1] = '\0';
        fclose(f);
        acpi_process_dev(acpi_path);
    }
    closedir(pci_base_dir);
    kos_acpi_print_tree(ctx);
    kos_acpi_fill_pci_irqs(ctx);
    dump_devices_dat("devices.dat");

    return 0;
}

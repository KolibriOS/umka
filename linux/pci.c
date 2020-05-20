#include <stdio.h>
#include <inttypes.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <limits.h>
#include "pci.h"

char pci_path[PATH_MAX] = ".";

__attribute__((stdcall)) uint32_t pci_read(uint32_t bus, uint32_t dev, uint32_t fun, uint32_t offset, size_t len) {
    char path[PATH_MAX*2];
    uint32_t value = 0;
    sprintf(path, "%s/%4.4u:%2.2u:%2.2u.%u/config", pci_path, 0, bus, dev, fun);
    int fd = open(path, O_RDONLY);
    if (!fd) {
        // TODO: report an error
        return UINT32_MAX;
    }
    lseek(fd, offset, SEEK_SET);
    read(fd, &value, len);
    close(fd);
    return value;
}

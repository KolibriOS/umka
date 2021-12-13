#include "pci.h"

#include <stdio.h>
#include <stdint.h>
#include "../umka.h"

char pci_path[UMKA_PATH_MAX] = ".";

__attribute__((stdcall)) uint32_t pci_read(uint32_t bus, uint32_t dev,
                                           uint32_t fun, uint32_t offset,
                                           size_t len) {
    printf("STUB: %s:%d", __FILE__, __LINE__);
    return 0xffffffff;
}

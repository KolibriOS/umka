#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include <errno.h>
#include "kolibri.h"
#include "trace.h"

typedef struct {
    unsigned x;
} vnet_t;

void *vnet_init() {
    vnet_t *vnet = (vnet_t*)malloc(sizeof(vnet_t));
    *vnet = (vnet_t){.x = 0,};
    return vnet;
}

__attribute__((__stdcall__))
void vnet_unload() {
    COVERAGE_OFF();
    COVERAGE_ON();
}

__attribute__((__stdcall__))
void vnet_reset() {
    COVERAGE_OFF();
    COVERAGE_ON();
}

__attribute__((__stdcall__))
void vnet_transmit() {
    COVERAGE_OFF();
    COVERAGE_ON();
}


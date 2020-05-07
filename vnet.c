#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include <errno.h>
#include "umka.h"
#include "trace.h"

typedef struct {
    unsigned x;
} vnet_t;

void *vnet_init() {
    printf("vnet_init\n");
    vnet_t *vnet = (vnet_t*)malloc(sizeof(vnet_t));
    *vnet = (vnet_t){.x = 0,};
    return vnet;
}

__attribute__((__stdcall__))
void vnet_unload() {
    printf("vnet_unload\n");
    COVERAGE_OFF();
    COVERAGE_ON();
}

__attribute__((__stdcall__))
void vnet_reset() {
    printf("vnet_reset\n");
    COVERAGE_OFF();
    COVERAGE_ON();
}

static void dump_net_buff(net_buff_t *buf) {
    for (size_t i = 0; i < buf->length; i++) {
        printf("%2.2x ", buf->data[i]);
    }
    putchar('\n');
}

__attribute__((__stdcall__))
void vnet_transmit(net_buff_t *buf) {
    printf("vnet_transmit: %d bytes\n", buf->length);
    dump_net_buff(buf);
    COVERAGE_OFF();
    COVERAGE_ON();
}


#ifndef VNET_H_INCLUDED
#define VNET_H_INCLUDED

#include <stdio.h>
#include <inttypes.h>
#include "umka.h"

void *vnet_init(void);

__attribute__((__stdcall__))
void vnet_unload(void);

__attribute__((__stdcall__))
void vnet_reset(void);

__attribute__((__stdcall__))
void vnet_transmit(net_buff_t *buf);

#endif  // VNET_H_INCLUDED

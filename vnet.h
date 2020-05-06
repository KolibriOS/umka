#ifndef VNET_H_INCLUDED
#define VNET_H_INCLUDED

#include <stdio.h>
#include <inttypes.h>
#include "kolibri.h"

void *vnet_init(void);

__attribute__((__stdcall__))
void vnet_unload(void);

__attribute__((__stdcall__))
void vnet_reset(void);

__attribute__((__stdcall__))
void vnet_transmit(void);

#endif  // VNET_H_INCLUDED

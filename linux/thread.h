#ifndef THREAD_H_INCLUDED
#define THREAD_H_INCLUDED

#include <inttypes.h>

uint32_t umka_new_sys_threads(void (*func)(void), void *stack, int type);

#endif  // THREAD_H_INCLUDED

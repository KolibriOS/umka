#include <inttypes.h>
#include <setjmp.h>
#include <stdio.h>
#include <stdlib.h>
#include "../umka.h"

sigjmp_buf trampoline;


__attribute__((__stdcall__))
uint32_t umka_new_sys_threads(void (*func)(void), void *stack, int type) {
    (void)type;
    (void)func;
    fprintf(stderr, "umka_new_sys_threads before\n");
//    kos_slot_base[kos_task_count].fpu_state = malloc(4096);
    fprintf(stderr, "kos_task_count: %d\n", kos_task_count);
    if (!sigsetjmp(trampoline, 1)) {
        __asm__ __inline__ __volatile__ (
        "mov    esp, eax;"
        "push   ecx"
        :
        : "a"(stack),
          "c"(func)
        : "memory");
        if (!sigsetjmp(*kos_slot_base[kos_task_count++].fpu_state, 1)) {
            longjmp(trampoline, 1);
        } else {
            __asm__ __inline__ __volatile__ (
            "pop    ecx;"
            "call   ecx"
            :
            :
            : "memory");
        }
    }
    fprintf(stderr, "umka_new_sys_threads after\n");
    return 0;
}


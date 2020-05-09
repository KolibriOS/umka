#include <inttypes.h>
#include <setjmp.h>
#include <stdio.h>
#include <stdlib.h>
#include "../umka.h"

sigjmp_buf trampoline;


__attribute__((__stdcall__))
uint32_t umka_sched_add_thread(appdata_t *app) {
    fprintf(stderr, "umka_new_sys_threads before\n");
    fprintf(stderr, "kos_task_count: %d\n", kos_task_count);
    if (!sigsetjmp(trampoline, 1)) {
        __asm__ __inline__ __volatile__ (
        "mov    esp, eax"
        :
        : "a"(app->saved_esp)
        : "memory");
        if (!sigsetjmp(*app->fpu_state, 1)) {
            longjmp(trampoline, 1);
        } else {
            __asm__ __inline__ __volatile__ (
            "ret"
            :
            :
            : "memory");
        }
    }
    fprintf(stderr, "umka_new_sys_threads after\n");
    return 0;
}


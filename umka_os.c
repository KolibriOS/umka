#include <setjmp.h>
#define __USE_GNU
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/select.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include "umka.h"
#include "thread.h"
#include "shell.h"

struct itimerval timeout = {.it_value = {.tv_sec = 0, .tv_usec = 10000}};


void scheduler(int signo, siginfo_t *info, void *context) {
    (void)signo;
    (void)info;
    (void)context;
//    printf("##### switching from task %u\n", kos_current_task);
    ucontext_t *ctx = context;
    if (!sigsetjmp(*kos_slot_base[kos_current_task].fpu_state, 1)) {
        if (ctx->uc_mcontext.__gregs[REG_EFL] & (1 << 21)) {
            kos_current_task += 1;
            if (kos_current_task > kos_task_count) {
                kos_current_task = 1;
            }
        } else {
//            printf("########## cli ############\n");
        }
        kos_current_slot = kos_slot_base + kos_current_task;
        kos_task_base = ((taskdata_t*)&kos_current_task) + kos_current_task;
//        printf("##### kos_current_task: %u\n", kos_current_task);
        setitimer(ITIMER_PROF, &timeout, NULL);
        siglongjmp(*kos_slot_base[kos_current_task].fpu_state, 1);
    }
}

void monitor() {
    fprintf(stderr, "Start monitor thread\n");
//    mkfifo("/tmp/umka.fifo.2u", 0644);
//    mkfifo("/tmp/umka.fifo.4u", 0644);
    FILE *fin = fopen("/tmp/umka.fifo.2u", "r");
    FILE *fout = fopen("/tmp/umka.fifo.4u", "w");
    if (!fin || !fout) {
        fprintf(stderr, "Can't open monitor files!\n");
        return;
    }
    run_test(fin, fout, 0);
/*
    while (1) {
        for (int i = 0; i < 10000000; i++) {}
        printf("6 monitor\n");
    }
*/
}

int main() {
    umka_tool = UMKA_OS;
    struct sigaction sa;

    sa.sa_sigaction = scheduler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_SIGINFO;

    if (sigaction(SIGPROF, &sa, NULL) == -1) {
        printf("Can't install signal handler!\n");
        return 1;
    }

    monitor_thread = monitor;

    umka_os();

    return 0;
}

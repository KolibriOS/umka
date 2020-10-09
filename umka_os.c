#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/select.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include "umka.h"
#include "shell.h"

#define MONITOR_THREAD_STACK_SIZE 0x100000

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
}

int main() {
    umka_tool = UMKA_OS;

    struct sigaction sa;
    sa.sa_sigaction = irq0;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_SIGINFO;

    if (sigaction(SIGPROF, &sa, NULL) == -1) {
        printf("Can't install signal handler!\n");
        return 1;
    }

    monitor_thread = monitor;

    kos_init();
    kos_stack_init();
    uint8_t *monitor_stack = malloc(MONITOR_THREAD_STACK_SIZE);
    umka_new_sys_threads(1, monitor, monitor_stack + MONITOR_THREAD_STACK_SIZE);

    raise(SIGPROF);

    osloop();   // doesn't return

    return 0;
}

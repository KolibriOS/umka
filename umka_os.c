#include <setjmp.h>
#include <signal.h>
#include <stdio.h>
#include <sys/time.h>
#include "umka.h"

#define THREAD_STACK_SIZE 0x2000
#define MAX_THREADS 16

struct itimerval timeout = {.it_value = {.tv_sec = 0, .tv_usec = 999}};
sigjmp_buf threads[MAX_THREADS], trampoline;
size_t cur_thread = 0;
size_t thread_cnt = 1;
uint8_t stack_os[THREAD_STACK_SIZE];
uint8_t stack_thrda[THREAD_STACK_SIZE];
uint8_t stack_thrdb[THREAD_STACK_SIZE];

static void add_new_thread(void (*func)(void), void *stack) {
    if (!sigsetjmp(trampoline, 1)) {
        __asm__ __inline__ __volatile__ (
        "mov    esp, eax;"
        "push   ecx"
        :
        : "a"(stack),
          "c"(func)
        : "memory");
        if (!sigsetjmp(threads[thread_cnt++], 1)) {
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
}

void scheduler(int signo, siginfo_t *info, void *context) {
    (void)signo;
    (void)info;
    (void)context;
    printf("##### cur_thread: %u\n", cur_thread);
    if (!sigsetjmp(threads[cur_thread], 1)) {
        printf("##### saved\n");
        cur_thread += 1;
        if (cur_thread == thread_cnt) {
            cur_thread = 0;
        }
        setitimer(ITIMER_PROF, &timeout, NULL);
        siglongjmp(threads[cur_thread], 1);
    }
}

void os() {
    printf("0 osidle begin\n");
    raise(SIGPROF);
    while (1) { // osloop
        for (int i = 0; i < 10000000; i++) ;
        printf("0 osloop\n");
        fflush(stdout);
    }
    printf("0 osidle end\n");
}

void thrda() {
    printf("1 thrd a begin\n");
    while (1) {
        for (int i = 0; i < 10000000; i++) ;
        printf("1 thrd a\n");
        fflush(stdout);
    }
    printf("1 thrd a end\n");
}

void thrdb() {
    printf("2 thrd b begin\n");
    while (1) {
        for (int i = 0; i < 10000000; i++) ;
        printf("2 thrd b\n");
        fflush(stdout);
    }
    printf("2 thrd b end\n");
}

int main() {
    struct sigaction sa;

    sa.sa_sigaction = scheduler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_SIGINFO;

    if (sigaction(SIGPROF, &sa, NULL) == -1) {
        printf("Can't install signal handler!\n");
        return 1;
    }

//    add_new_thread(os,    stack_os[0x1000]);
    add_new_thread(thrda, stack_thrda+THREAD_STACK_SIZE);
    add_new_thread(thrdb, stack_thrdb+THREAD_STACK_SIZE);

    os();

    return 0;
}

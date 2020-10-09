#include <setjmp.h>
#define __USE_GNU
#include <signal.h>
#include <stddef.h>
#include <stdio.h>
#include <sys/time.h>

sigset_t mask;
struct itimerval timeout = {.it_value = {.tv_sec = 0, .tv_usec = 10000}};

void reset_procmask(void) {
    sigemptyset (&mask);
	sigaddset (&mask, SIGPROF);
    sigprocmask(SIG_UNBLOCK, &mask, NULL);
}

int get_fake_if(ucontext_t *ctx) {
    // we fake IF with id flag
    return ctx->uc_mcontext.__gregs[REG_EFL] & (1 << 21);
}

void restart_timer(void) {
    setitimer(ITIMER_PROF, &timeout, NULL);
}

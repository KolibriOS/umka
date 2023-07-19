/*
    SPDX-License-Identifier: GPL-2.0-or-later

    UMKa - User-Mode KolibriOS developer tools
    runtests - the test runner

    Copyright (C) 2023  Ivan Baravy <dunkaist@gmail.com>
*/

#include <dirent.h>
#include <errno.h>
#include <inttypes.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <pthread.h>
#include <sys/types.h>

#include "optparse/optparse.h"

#ifndef PATH_MAX
#define PATH_MAX 0x1000
#endif

#define CMPFILE_BUF_LEN 0x100000
#define TIMEOUT_FILENAME "timeout.txt"
#define TIMEOUT_STR_MAX_LEN 16
#define ACTION_RUN 0

_Thread_local char bufa[CMPFILE_BUF_LEN];
_Thread_local char bufb[CMPFILE_BUF_LEN];
_Thread_local char timeoutfname[PATH_MAX];
_Thread_local char reffname[PATH_MAX];
_Thread_local char outfname[PATH_MAX];

int silent_success = 1;

static int
is_valid_test_name(const char *name) {
    if (name[0] != 't') {
        return 0;
    }
    for (size_t i = 1; i < 4 && name[i]; i++) {
        if (name[1] < '0' || name[1] > '9') {
            return 0;
        }
    }
    return 1;
}

static int
cmpfiles(const char *fnamea, const char *fnameb) {
    int result = 0;
    FILE *filea = fopen(fnamea, "rb");
    if (!filea) {
        fprintf(stderr, "Can't open file '%s' for reading: %s\n", fnamea,
                strerror(errno));
        return -1;
    }
    FILE *fileb = fopen(fnameb, "rb");
    if (!fileb) {
        fprintf(stderr, "Can't open file '%s' for reading: %s\n", fnameb,
                strerror(errno));
        fclose(filea);
        return -1;
    }
    while (1) {
        size_t reada = fread(bufa, 1, CMPFILE_BUF_LEN, filea);
        size_t readb = fread(bufb, 1, CMPFILE_BUF_LEN, fileb);
        if (reada != readb || memcmp(bufa, bufb, reada)) {
            fprintf(stderr, "[!] files %s and %s don't match\n", fnamea,
                    fnameb);
            result = -1;
            break;
        }
        if (feof(filea) && feof(fileb)) {
            break;
        }
    }
    fclose(filea);
    fclose(fileb);
    return result;
}

static int
check_test_artefacts(const char *testname) {
    DIR *tdir = opendir(testname);
    if (!tdir) {
        fprintf(stderr, "Can't open directory '%s': %s\n", testname,
                strerror(errno));
        return -1;
    }

    struct dirent *dent;
    while ((dent = readdir(tdir))) {
        const char *refdot = strstr(dent->d_name, "ref.");
        if (!refdot) {
            continue;
        }
        size_t ref_off = refdot - dent->d_name;
        sprintf(reffname, "%s/%s", testname, dent->d_name);
        strcpy(outfname, reffname);
        memcpy(outfname + strlen(testname) + 1 + ref_off, "out", 3);
        if (cmpfiles(reffname, outfname)) {
            return -1;
        }
    }
    closedir(tdir);
    return 0;
}

struct test_wait_arg {
    int pid;
    pthread_cond_t *cond;
};

static unsigned
get_test_timeout(const char *testname) {
    sprintf(timeoutfname, "%s/%s", testname, TIMEOUT_FILENAME);
    FILE *f = fopen(timeoutfname, "rb");
    if (!f) {
        fprintf(stderr, "[!] Can't open %s\n: %s\n", TIMEOUT_FILENAME,
                strerror(errno));
        return 0;
    }
    char str[TIMEOUT_STR_MAX_LEN];
    fread(str, 1, TIMEOUT_STR_MAX_LEN, f);
    fclose(f);
    unsigned n, sec = 0, min = 0;
    char *end;
    n = strtoul(str, &end, 0);
    switch (*end) {
    case 's':
        sec = n;
        break;
    case 'm':
        min = n;
        sec = strtoul(end+1, &end, 0);
        break;
    default:
        fprintf(stderr, "[!] bad timeout value: %s\n", str);
        return 0;
    }
    return min*60 + sec;
}

#ifndef _WIN32

#include <sys/wait.h>

static void *
thread_wait(void *arg) {
    struct test_wait_arg *wa = arg;
    int status;
    waitpid(wa->pid, &status, 0);
    pthread_cond_signal(wa->cond);
    return (void *)(intptr_t)status;
}

static void *
run_test(const void *arg) {
    const char *test_name = arg;
    void *result = NULL;
    pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
    pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
    pthread_mutex_lock(&mutex);
    int child;
    if (!(child = fork())) {
        chdir(test_name);
        execl("../../umka_shell", "../../umka_shell", "-ri", "run.us", "-o",
              "out.log", NULL);
        fprintf(stderr, "Can't run test command: %s\n", strerror(errno));
        return (void *)-1;
    }
    pthread_t t;
    struct test_wait_arg wa = {.pid = child, .cond = &cond};
    pthread_create(&t, NULL, thread_wait, &wa);
    unsigned tout = get_test_timeout(test_name);
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    ts.tv_sec += tout;
    int status;
    if ((status = pthread_cond_timedwait(&cond, &mutex, &ts))) {
        fprintf(stderr, "[!] %s: timeout (%um%us)\n", test_name,
                tout/60, tout % 60);
        kill(child, SIGKILL);
        result = (void *)(intptr_t)status;
    }
    pthread_join(t, &result);
    pthread_mutex_unlock(&mutex);
    
    return result;
}
#else

#include <errhandlingapi.h>
#include <intsafe.h>
#include <io.h>
#include <processthreadsapi.h>
#include <synchapi.h>

static void *
run_test(const void *arg) {
    const char *test_name = arg;
    void *result = NULL;
    STARTUPINFO si;
    PROCESS_INFORMATION pi;
    memset(&si, 0, sizeof(si));
    memset(&pi, 0, sizeof(pi));
    unsigned tout = get_test_timeout(test_name);

    if(!CreateProcessA(NULL, "../umka_shell -ri run.us -o out.log", NULL,
                       NULL, FALSE, 0, NULL, test_name, &si, &pi)) {
        fprintf(stderr, "CreateProcess failed: %lu\n", GetLastError());
        return (void *)-1;
    }

    DWORD wait_res = WaitForSingleObject(pi.hProcess, tout*1000);
    if (wait_res == WAIT_TIMEOUT) {
        TerminateProcess(pi.hProcess, 1);
        result = (void *)(intptr_t)-1;
    }
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    return result;
}
#endif

pthread_mutex_t readdir_mutex = PTHREAD_MUTEX_INITIALIZER;

static void *
thread_run_test(void *arg) {
    DIR *cwd = arg;
    struct dirent *dent;
    while (1) {
        pthread_mutex_lock(&readdir_mutex);
        dent = readdir(cwd);
        pthread_mutex_unlock(&readdir_mutex);
        if (!dent) {
            break;
        }
        const char *testname = dent->d_name;
        if (!is_valid_test_name(testname)) {
            continue;
        }
        fprintf(stderr, "running test %s\n", testname);
        if (run_test(testname) || check_test_artefacts(testname)) {
            fprintf(stderr, "[!] test %s failed\n", testname);
        } else if (!silent_success) {
                fprintf(stderr, "test %s ok\n", testname);
        }
    }
    return NULL;
}

int
main(int argc, char *argv[]) {
    (void)argc;
//    int action = ACTION_RUN;
    size_t nthreads = 1;

    struct optparse opts;
    optparse_init(&opts, argv);
    int opt;

    while ((opt = optparse(&opts, "j:s")) != -1) {
        switch (opt) {
        case 'j':
            nthreads = strtoul(opts.optarg, NULL, 0);
            break;
        case 's':
            silent_success = 0;
            break;
        default:
            fprintf(stderr, "[!] Unknown option: '%c'\n", opt);
            exit(1);
        }
    }


    DIR *cwd = opendir(".");
    pthread_t *threads = malloc(sizeof(pthread_t)*nthreads);
    for (size_t i = 0; i < nthreads; i++) {
        pthread_create(threads + i, NULL, thread_run_test, cwd);
    }
    for (size_t i = 0; i < nthreads; i++) {
        void *retval;
        pthread_join(threads[i], &retval);
    }
    free(threads);
    return 0;
}

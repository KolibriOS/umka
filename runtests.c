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
#define TAGS_FILENAME "tags.txt"
#define TIMEOUT_FILENAME "timeout.txt"
#define TAGS_STR_MAX_LEN 1024
#define TIMEOUT_STR_MAX_LEN 16

_Thread_local char bufa[CMPFILE_BUF_LEN];
_Thread_local char bufb[CMPFILE_BUF_LEN];
_Thread_local char tagsfname[PATH_MAX];
_Thread_local char timeoutfname[PATH_MAX];
_Thread_local char reffname[PATH_MAX];
_Thread_local char outfname[PATH_MAX];

int coverage = 0;
int no_timeout = 0;
int silent_success = 1;

enum tag_mode {
    TAG_ENABLED,
    TAG_DISABLED,
};

struct tag;
struct tag {
    struct tag *next;
    const char *name;
    enum tag_mode mode;
};

struct tag *user_tags = NULL;

/*
static void
dump_tags(const struct tag *tags) {
    printf("dumping tags\n");
    for (const struct tag *t = tags; t; t = t->next) {
        printf("tag: %s (%i)\n", t->name, t->mode);
    }
}
*/

static void
add_tag(struct tag **root, struct tag *t) {
    t->next = *root;
    *root = t;
}

static void
parse_add_tag(struct tag **tags, const char *prefix, const char *s) {
    char name[32];
    struct tag *t = malloc(sizeof(struct tag));
    if (!t) {
        return;
    }
    if (s[0] == '-') {
        t->mode = TAG_DISABLED;
        s += 1;
    } else {
        t->mode = TAG_ENABLED;
    }
    if (prefix) {
        sprintf(name, "%s_%s", prefix, s);
    } else {
        sprintf(name, "%s", s);
    }
    t->name = strdup(name);
    add_tag(tags, t);
    return;
}

static void
parse_tags(struct tag **tags, char *str) {
    for (char *asave = str, *as; (as = strtok_r(asave, " \n\r\t", &asave));) {
        char *colon = strchr(as, ':');
        if (!colon) {
            for (char *csave = as, *cs; (cs = strtok_r(csave, ",", &csave));) {
                parse_add_tag(tags, NULL, cs);
            }
        } else {
            *colon = '\0';
            parse_add_tag(tags, NULL, as);
            for (char *csave = colon+1, *cs; (cs = strtok_r(csave, ",", &csave));) {
                parse_add_tag(tags, as, cs);
            }
        }
    }
}

static bool
match_tags(const struct tag *user, const struct tag *test) {
    if (!user || !test) {
        return true;
    }
    for (const struct tag *ut = user; ut; ut = ut->next) {
        for (const struct tag *tt = test; tt; tt = tt->next) {
            if ((ut->mode == TAG_ENABLED) && !strcmp(ut->name, tt->name)) {
                return true;
            }
        }
    }
    return false;
}

static int
is_valid_test(const char *name) {
    // Check that name starts with t\d\d\d
    if (name[0] != 't') {
        return 0;
    }
    for (size_t i = 1; i < 4 && name[i]; i++) {
        if (name[i] < '0' || name[i] > '9') {
            return 0;
        }
    }
    // Check that run.us file exists
    sprintf(reffname, "%s/%s", name, "run.us");
    FILE *f = fopen(reffname, "rb");
    if (f) {
        fclose(f);
    }
    return f != NULL;
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
    pthread_mutex_t *mutex;
};

static time_t
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

static struct tag *
get_test_tags(const char *testname) {
    sprintf(tagsfname, "%s/%s", testname, TAGS_FILENAME);
    char str[TAGS_STR_MAX_LEN];
    FILE *f = fopen(tagsfname, "rb");
    if (!f) {   // it's okay to not have a tags.txt file
        return NULL;
    }
    size_t nread = fread(str, 1, TAGS_STR_MAX_LEN - 1, f);
    str[nread] = '\0';
    fclose(f);
    struct tag *test_tags = NULL;
    parse_tags(&test_tags, str);
//    dump_tags(test_tags);
    return test_tags;
}

#ifndef _WIN32

#include <sys/wait.h>

static void *
thread_wait(void *arg) {
    struct test_wait_arg *wa = arg;
    int status;
    pthread_mutex_lock(wa->mutex);
    waitpid(wa->pid, &status, 0);
    pthread_cond_signal(wa->cond);
    pthread_mutex_unlock(wa->mutex);
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
        if (coverage) {
            char covfile[64];
            sprintf(covfile, "../cov_%s", test_name);
            execl("/usr/bin/taskset", "taskset", "1", "sudo",
                  "../../umka_shell", "-ri", "run.us", "-o", "out.log", "-c",
                  covfile, NULL);
        } else {
            execl("../../umka_shell", "../../umka_shell", "-ri", "run.us", "-o",
                "out.log", NULL);
        }
        fprintf(stderr, "Can't run test command: %s\n", strerror(errno));
        return (void *)-1;
    }
    pthread_t t;
    struct test_wait_arg wa = {.pid = child, .cond = &cond, .mutex = &mutex};
    pthread_create(&t, NULL, thread_wait, &wa);
    time_t tout = get_test_timeout(test_name);
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    if (no_timeout) {
        ts.tv_sec += 60*60*24*7;  // a week
    } else {
        ts.tv_sec += tout;
    }
    int status;
    if ((status = pthread_cond_timedwait(&cond, &mutex, &ts))) {
        fprintf(stderr, "[!] %s: timeout (%llim%llis)\n", test_name,
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
        if (!is_valid_test(testname)) {
            continue;
        }
        struct tag *test_tags = get_test_tags(testname);
        if (!match_tags(user_tags, test_tags)) {
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
    const char *usage = \
        "usage: runtests [-c] [-f] [-h] [-j] [-s]\n"
        "  -c               collect coverage\n"
        "  -f               run without timeouts (forever)\n"
        "  -h               show this help\n"
        "  -j<n>            run <n> threads in parallel\n"
        "  -s               repost each successful test\n"
        "  -t               run only specific tests\n";

    size_t nthreads = 1;
    struct optparse opts;
    optparse_init(&opts, argv);
    int opt;

    while ((opt = optparse(&opts, "cfhj:st:")) != -1) {
        switch (opt) {
        case 'c':
            coverage = 1;
            break;
        case 'f':
            no_timeout = 1;
            break;
        case 'h':
            fputs(usage, stderr);
            exit(EXIT_SUCCESS);
            break;
        case 'j':
            nthreads = strtoul(opts.optarg, NULL, 0);
            break;
        case 's':
            silent_success = 0;
            break;
        case 't':
            parse_tags(&user_tags, opts.optarg);
//            dump_tags(user_tags);
            break;
        default:
            fprintf(stderr, "[!] %s: %s\n", argv[0], opts.errmsg);
            exit(EXIT_FAILURE);
        }
    }

    DIR *cwd = opendir(".");
    pthread_t *threads = malloc(sizeof(pthread_t)*nthreads);
    if (!threads) {
        return 1;
    }
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

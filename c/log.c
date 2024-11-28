//
// Created by 韦轩 on 2024/11/26.
//

#include "log.h"

#include <pthread.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>

#include <sys/types.h>

#ifdef __APPLE__
_Thread_local uint64_t tid = 0;
#else
_Thread_local pthread_t tid = 0;
#endif

void timef(const char * fmt, ...) {
    if (tid == 0) {
#ifdef __APPLE__
        pthread_threadid_np(pthread_self(), &tid);
#else
        tid = pthread_self();
#endif
    }

    time_t rawtime = 0;
    time(&rawtime);
    struct tm tm;
    localtime_r(&rawtime, &tm);

    char time_buf[64];
    strftime(time_buf, sizeof(time_buf), "[%Y-%m-%d %H:%M:%S]", &tm);

    char buf[256];
#ifdef __APPLE__
    snprintf(buf, sizeof(buf), "%s [%llu] ", time_buf, tid);
#else
    snprintf(buf, sizeof(buf), "%s [%lu] ", time_buf, tid);
#endif

    fprintf(stderr, "%s", buf);

    va_list args;
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);
}

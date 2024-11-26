//
// Created by 韦轩 on 2024/11/26.
//

#include "log.h"
#include <pthread.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>
#ifdef __APPLE__
_Thread_local uint64_t tid = 0;
#else
_Thread_local pid_t tid;
#endif

void timef(const char *fmt, ...) {
#ifdef __APPLE__
  if (tid == 0) {
    pthread_threadid_np(pthread_self(), &tid);
  }
#else
  tid = gettid();
#endif
  time_t rawtime;
  time(&rawtime);
  struct tm tm;
  localtime_r(&rawtime, &tm);

  char time_buf[64];
  strftime(time_buf, sizeof(time_buf), "[%Y-%m-%d %H:%M:%S]", &tm);

  char final_buf[256];
  snprintf(final_buf, sizeof(final_buf), "%s [%lu] ", time_buf, tid);

  fprintf(stderr, "%s", final_buf);

  va_list args;
  va_start(args, fmt);
  vfprintf(stderr, fmt, args);
  va_end(args);
}
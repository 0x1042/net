//
// Created by 韦轩 on 2024/11/26.
//

#include "log.h"
#include <pthread.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <time.h>

_Thread_local uint64_t tid = 0;

void timef(const char *fmt, ...) {
  if (tid == 0) {
    pthread_threadid_np(pthread_self(), &tid);
  }

  time_t rawtime;
  time(&rawtime);
  struct tm tm;
  localtime_r(&rawtime, &tm);

  char time_buf[64];
  strftime(time_buf, sizeof(time_buf), "[%Y-%m-%d %H:%M:%S]", &tm);

  char final_buf[256];
  snprintf(final_buf, sizeof(final_buf), "%s [%llu] ", time_buf, tid);

  fprintf(stderr, "%s", final_buf);

  va_list args;
  va_start(args, fmt);
  vfprintf(stderr, fmt, args);
  va_end(args);
}
//
// Created by 韦轩 on 2024/11/26.
//

#pragma once

#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

static const char * const COLOR_WHITE = "\033[37m";
static const char * const COLOR_CYAN = "\033[36m";
static const char * const COLOR_BLUE = "\033[34m";
static const char * const COLOR_YELLOW = "\033[33m";
static const char * const COLOR_RED = "\033[31m";
static const char * const COLOR_RESET = "\033[0m";

typedef enum {
    LEVEL_TRACE = -2,
    LEVEL_DEBUG = -1,
    LEVEL_INFO = 0,
    LEVEL_WARN = 1,
    LEVEL_ERROR = 2,
} level_t;

void timef(level_t level, const char * fmt, ...);

void set_level(level_t level);

#define LOG_FMT(level, color, type, fmt, ...) \
    timef(level, "%s [" type "] " fmt "%s\n", color, ##__VA_ARGS__, COLOR_RESET)

#define TRACE(...) LOG_FMT(LEVEL_TRACE, COLOR_WHITE, "TRACE", __VA_ARGS__)
#define DEBUG(...) LOG_FMT(LEVEL_DEBUG, COLOR_CYAN, "DEBUG", __VA_ARGS__)
#define INFO(...) LOG_FMT(LEVEL_INFO, COLOR_BLUE, "INFO", __VA_ARGS__)
#define WARN(...) LOG_FMT(LEVEL_WARN, COLOR_YELLOW, "WARN", __VA_ARGS__)
#define ERROR(...) LOG_FMT(LEVEL_ERROR, COLOR_RED, "ERROR", __VA_ARGS__)

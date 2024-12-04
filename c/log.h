//
// Created by 韦轩 on 2024/11/26.
//

#pragma once

#define COLOR_CYAN "\x1B[36m"
#define COLOR_RED "\x1B[31m"
#define COLOR_YELLOW "\x1B[33m"
#define COLOR_BLUE "\x1B[34m"
#define COLOR_WHITE "\x1B[37m"
#define COLOR_RESET "\x1B[0m"

typedef enum {
    LEVEL_TRACE = -2,
    LEVEL_DEBUG = -1,
    LEVEL_INFO = 0,
    LEVEL_WARN = 1,
    LEVEL_ERROR = 2,
} level_t;

void timef(level_t level, const char * fmt, ...);

void set_level(level_t level);

#define TRACE(fmt, ...) timef(LEVEL_TRACE, "%s [TRACE] " fmt "%s\n", COLOR_WHITE, ##__VA_ARGS__, COLOR_RESET)

#define DEBUG(fmt, ...) timef(LEVEL_DEBUG, "%s [DEBUG] " fmt "%s\n", COLOR_CYAN, ##__VA_ARGS__, COLOR_RESET)

#define INFO(fmt, ...) timef(LEVEL_INFO, "%s [INFO] " fmt "%s\n", COLOR_BLUE, ##__VA_ARGS__, COLOR_RESET)

#define WARN(fmt, ...) timef(LEVEL_WARN, "%s [WARN] " fmt "%s\n", COLOR_YELLOW, ##__VA_ARGS__, COLOR_RESET)

#define ERROR(fmt, ...) timef(LEVEL_ERROR, "%s [ERROR] " fmt "%s\n", COLOR_RED, ##__VA_ARGS__, COLOR_RESET)

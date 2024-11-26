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

void timef(const char *fmt, ...);

// #define TRACE(fmt, ...)                                                        \
//   timef("%s[TRACE ] %10s:%10d:%20s(): " fmt "\n%s", COLOR_RESET, __FILE__,     \
//         __LINE__, __func__, ##__VA_ARGS__, COLOR_RESET)

// #define INFO(fmt, ...)                                                         \
//   timef("%s[INFO ] %10s:%10d:%20s(): " fmt "%s\n", COLOR_BLUE, __FILE__,       \
//         __LINE__, __func__, ##__VA_ARGS__, COLOR_RESET)

#define INFO(fmt, ...)                                                         \
  timef("%s [INFO] " fmt "%s\n", COLOR_BLUE, ##__VA_ARGS__, COLOR_RESET)

#define TRACE(fmt, ...)                                                        \
  timef("%s [TRACE] " fmt "%s\n", COLOR_WHITE, ##__VA_ARGS__, COLOR_RESET)

#define ERROR(fmt, ...)                                                        \
  timef("%s [ERROR] " fmt "%s\n", COLOR_RED, ##__VA_ARGS__, COLOR_RESET)

#define DEBUG(fmt, ...)                                                        \
  timef("%s [DEBUG] " fmt "%s\n", COLOR_CYAN, ##__VA_ARGS__, COLOR_RESET)

#define WARN(fmt, ...)                                                        \
  timef("%s [WARN] " fmt "%s\n", COLOR_YELLOW, ##__VA_ARGS__, COLOR_RESET)
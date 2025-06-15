#pragma once
#include <spdlog/spdlog.h>

#define DEBUG(...)                                                           \
    do {                                                                     \
        if (auto logger = spdlog::default_logger_raw();                      \
            logger != nullptr && logger->should_log(spdlog::level::debug)) { \
            SPDLOG_DEBUG(__VA_ARGS__);                                       \
        }                                                                    \
    } while (0)

#define INFO(...)                                                           \
    do {                                                                    \
        if (auto logger = spdlog::default_logger_raw();                     \
            logger != nullptr && logger->should_log(spdlog::level::info)) { \
            SPDLOG_INFO(__VA_ARGS__);                                       \
        }                                                                   \
    } while (0)

#define WARN(...)                                                           \
    do {                                                                    \
        if (auto logger = spdlog::default_logger_raw();                     \
            logger != nullptr && logger->should_log(spdlog::level::warn)) { \
            SPDLOG_WARN(__VA_ARGS__);                                       \
        }                                                                   \
    } while (0)

#define ERROR(...)                                                         \
    do {                                                                   \
        if (auto logger = spdlog::default_logger_raw();                    \
            logger != nullptr && logger->should_log(spdlog::level::err)) { \
            SPDLOG_ERROR(__VA_ARGS__);                                     \
        }                                                                  \
    } while (0)

#define CRITICAL(...)                                                           \
    do {                                                                        \
        if (auto logger = spdlog::default_logger_raw();                         \
            logger != nullptr && logger->should_log(spdlog::level::critical)) { \
            SPDLOG_CRITICAL(__VA_ARGS__);                                       \
        }                                                                       \
    } while (0)
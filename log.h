// log.h
#ifndef __LOG_H__
#define __LOG_H__

#include <stdio.h>
#include <time.h>

// 定义日志级别
#define LOG_LEVEL_ERROR 1
#define LOG_LEVEL_WARN  2
#define LOG_LEVEL_INFO  3
#define LOG_LEVEL_DEBUG 4

// 设置当前日志级别（可以通过编译参数控制）
#ifndef CURRENT_LOG_LEVEL
#define CURRENT_LOG_LEVEL LOG_LEVEL_DEBUG
#endif

// 带时间戳的日志打印
#define LOG(level, fmt, ...) \
    do { \
        if (level <= CURRENT_LOG_LEVEL) { \
            time_t now = time(NULL); \
            char *time_str = ctime(&now); \
            time_str[strlen(time_str)-1] = '\0'; \
            printf("[%s] " fmt "\n", time_str, ##__VA_ARGS__); \
        } \
    } while(0)

#define LOG_ERROR(fmt, ...) LOG(LOG_LEVEL_ERROR, "[ERROR] " fmt, ##__VA_ARGS__)
#define LOG_WARN(fmt, ...)  LOG(LOG_LEVEL_WARN,  "[WARN] "  fmt, ##__VA_ARGS__)
#define LOG_INFO(fmt, ...)  LOG(LOG_LEVEL_INFO,  "[INFO] "  fmt, ##__VA_ARGS__)
#define LOG_DEBUG(fmt, ...) LOG(LOG_LEVEL_DEBUG, "[DEBUG] " fmt, ##__VA_ARGS__)

#endif
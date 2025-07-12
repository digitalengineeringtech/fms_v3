/**
 * @file fms_debug.h
 * @brief Debug and logging utilities for the FMS system
 * @version 0.1.0
 * @date 2025
 */

#ifndef FMS_DEBUG_H
#define FMS_DEBUG_H

#include <Arduino.h>

/**
 * @brief Enumeration of log levels
 */
enum FMSLogLevel {
    FMS_LOG_NONE = 0,
    FMS_LOG_ERROR,
    FMS_LOG_WARNING,
    FMS_LOG_INFO,
    FMS_LOG_DEBUG,
    FMS_LOG_VERBOSE,
    FMS_LOG_TASK
};
void fmsLog(FMSLogLevel level, const char* format, ...);
#define FMS_LOG_ERROR(format, ...)    fmsLog(FMS_LOG_ERROR, format, ##__VA_ARGS__)
#define FMS_LOG_WARNING(format, ...)  fmsLog(FMS_LOG_WARNING, format, ##__VA_ARGS__)
#define FMS_LOG_INFO(format, ...)     fmsLog(FMS_LOG_INFO, format, ##__VA_ARGS__)
#define FMS_LOG_DEBUG(format, ...)    fmsLog(FMS_LOG_DEBUG, format, ##__VA_ARGS__)
#define FMS_LOG_VERBOSE(format, ...)  fmsLog(FMS_LOG_VERBOSE, format, ##__VA_ARGS__)
#define FMS_LOG_TASK(format, ...)     fmsLog(FMS_LOG_TASK, format, ##__VA_ARGS__)

void fmsSetLogLevel(FMSLogLevel level);
FMSLogLevel fmsGetLogLevel();
void fmsEnableSerialLogging(bool enable);
void fmsEnableSDLogging(bool enable);
void fmsSetLogFilePath(const char* path);
#endif /* FMS_DEBUG_H */


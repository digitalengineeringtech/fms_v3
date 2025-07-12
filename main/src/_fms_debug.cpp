/**
 * @file fms_debug.cpp
 * @brief Debug and logging implementation for the FMS system
 * @version 0.1.0
 * @date 2025
 */

#include "_fms_debug.h"
#include <Arduino.h>
#include <stdarg.h>

static FMSLogLevel currentLogLevel = FMS_LOG_INFO;
static bool logToSerial = true;
static bool logToSD = false;
static String logFilePath = "/logs/fms.log";
extern int s_uart_debug_nr= 0;

int log_printfv(const char *format, va_list arg) {
  static char loc_buf[64];
  char *temp = loc_buf;
  uint32_t len;
  va_list copy;
  va_copy(copy, arg);
  len = vsnprintf(NULL, 0, format, copy);
  va_end(copy);
  if (len >= sizeof(loc_buf)) {
    temp = (char *)malloc(len + 1);
    if (temp == NULL) {
      return 0;
    }
  }
  
  // Note: Mutex locks are commented out to avoid deadlocks with logging
  // in specific cases and with C++ constructors that may send logs
  
#if (ARDUINO_USB_CDC_ON_BOOT == 1 && ARDUINO_USB_MODE == 0) || CONFIG_IDF_TARGET_ESP32C3 \
  || ((CONFIG_IDF_TARGET_ESP32H2 || CONFIG_IDF_TARGET_ESP32C6 || CONFIG_IDF_TARGET_ESP32P4) && ARDUINO_USB_CDC_ON_BOOT == 1)
  vsnprintf(temp, len + 1, format, arg);
  ets_printf("%s", temp);
#else
  int wlen = vsnprintf(temp, len + 1, format, arg);
  for (int i = 0; i < wlen; i++) {
    ets_write_char_uart(temp[i]);
  }
#endif

  if (len >= sizeof(loc_buf)) {
    free(temp);
  }
  return len;
}

void fmsLog(FMSLogLevel level, const char* format, ...) {
    if (level > currentLogLevel) {
        return;
    }
    
    // Get current time
    char timeStr[20];
    unsigned long ms = millis();
    unsigned long seconds = ms / 1000;
    unsigned long minutes = seconds / 60;
    unsigned long hours = minutes / 60;
    
    snprintf(timeStr, sizeof(timeStr), "%02lu:%02lu:%02lu.%03lu", 
             hours, minutes % 60, seconds % 60, ms % 1000);
    
    // Get level string
    const char* levelStr;
    switch (level) {
        case FMS_LOG_ERROR:   levelStr = "ERROR"; break;
        case FMS_LOG_WARNING: levelStr = "WARN"; break;
        case FMS_LOG_INFO:    levelStr = "INFO"; break;
        case FMS_LOG_DEBUG:   levelStr = "DEBUG"; break;
        case FMS_LOG_VERBOSE: levelStr = "VERB"; break;
        case FMS_LOG_TASK:    levelStr = "TASK"; break;
        default:              levelStr = "NONE"; break;
    }
    char prefix[64];
    snprintf(prefix, sizeof(prefix), "[%s] [%s] ", timeStr, levelStr);
    if (logToSerial) {
        log_printf("%s", prefix);
        va_list args;
        va_start(args, format);
        log_printfv(format, args);
        va_end(args);
        log_printf("\n");
    }
    
    // Log to SD card
    
}
void fmsSetLogLevel(FMSLogLevel level) {
    currentLogLevel = level;
}

FMSLogLevel fmsGetLogLevel() {
    return currentLogLevel;
}

void fmsEnableSerialLogging(bool enable) {
    logToSerial = enable;
}
void fmsEnableSDLogging(bool enable) {
    logToSD = enable;
}

void fmsSetLogFilePath(const char* path) {
    logFilePath = path;
}


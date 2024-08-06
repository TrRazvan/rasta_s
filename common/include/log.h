#ifndef LOG_H
#define LOG_H

#include <stdio.h>
#include <time.h>
#include <string.h>

#define STRINGIFY(x) #x

/* Log levels */
typedef enum {
    LOG_DEBUG,
    LOG_INFO,
    LOG_WARNING,
    LOG_ERROR
} LogLevel;

// #define LOG_LEVEL LOG_DEBUG  /* Set the default log level here */
LogLevel get_loglevel_filter(void);
void set_loglevel_filter(LogLevel level);

/* Get current time as string */
static inline const char* current_time_str() {
    static char buffer[20];
    time_t t;
    time(&t);
    struct tm* tm_info = localtime(&t);
    strftime(buffer, 20, "%Y-%m-%d %H:%M:%S", tm_info);
    return buffer;
}

#define __FILENAME__ (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)

/* Log macro */
#define LOG(level, fmt, ...) \
    do { \
        if (level >= get_loglevel_filter()) { \
            fprintf(stderr, "[%s] %s:%d: " fmt "\n", current_time_str(), __FILENAME__, __LINE__, ##__VA_ARGS__); \
        } \
    } while (0)

/* Convenience macros for different log levels */
#define LOG_DEBUG(fmt, ...) LOG(LOG_DEBUG, "DEBUG: " fmt, ##__VA_ARGS__)
#define LOG_INFO(fmt, ...) LOG(LOG_INFO, "INFO: " fmt, ##__VA_ARGS__)
#define LOG_WARNING(fmt, ...) LOG(LOG_WARNING, "WARNING: " fmt, ##__VA_ARGS__)
#define LOG_ERROR(fmt, ...) LOG(LOG_ERROR, "ERROR: " fmt, ##__VA_ARGS__)

#define TRACE_ENTRY(fmt, ...) LOG_INFO(fmt, ##__VA_ARGS__)

#endif /* LOG_H */

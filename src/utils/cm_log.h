#ifndef CM_LOG_H
#define CM_LOG_H

#ifndef LOG_LEVEL_ENABLED
#define LOG_LEVEL_ENABLED 5
#endif

#include <stdarg.h>
#include <stdlib.h>

#ifndef LOG_SUBSYSTEM
extern const char *LOG_SUBSYSTEM;
#endif

enum LOG_SEVERITY {
	LOG_FATAL = 1,
	LOG_ERROR = 2,
	LOG_WARN = 3,
	LOG_INFO = 4,
	LOG_DEBUG = 5,
	LOG_TRACE = 6
};

int log_configure(int argc, char *argv[]);

void log_write(int level, const char *subsystem,
	       const char *format, ...);


#if LOG_LEVEL_ENABLED >= 6
#define log_trace(format, ...) \
	log_write(LOG_TRACE, LOG_SUBSYSTEM, format, ##__VA_ARGS__);
#else
#define log_trace(format, ...)
#endif

#if LOG_LEVEL_ENABLED >= 5
#define log_debug(format, ...) \
	log_write(LOG_DEBUG, LOG_SUBSYSTEM, format, ##__VA_ARGS__);
#else
#define log_debug(format, ...)
#endif

#if LOG_LEVEL_ENABLED >= 4
#define log_info(format, ...) \
	log_write(LOG_INFO, LOG_SUBSYSTEM, format, ##__VA_ARGS__);
#else
#define log_info(format, ...)
#endif

#if LOG_LEVEL_ENABLED >= 3
#define log_warn(format, ...) \
	log_write(LOG_WARN, LOG_SUBSYSTEM, format, ##__VA_ARGS__);
#else
#define log_warn(format, ...)
#endif

#if LOG_LEVEL_ENABLED >= 2
#define log_error(format, ...) \
	log_write(LOG_ERROR, LOG_SUBSYSTEM, format, ##__VA_ARGS__);
#else
#define log_error(format, ...)
#endif

#if LOG_LEVEL_ENABLED >= 1
#define log_fatal(format, ...) \
	log_write(LOG_FATAL, LOG_SUBSYSTEM, format, ##__VA_ARGS__);
#else
#define log_fatal(format, ...)
#endif


#endif // CM_LOG_H

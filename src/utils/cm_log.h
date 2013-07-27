#ifndef CM_LOG_H
#define CM_LOG_H

#define LOG_LEVEL_ENABLED 5

#include <stdarg.h>
#include <stdlib.h>

#ifndef LOG_SUBSYSTEM
#define LOG_SUBSYSTEM ""
#endif

enum LOG_SEVERITY {
	LOG_FATAL = 1,
	LOG_ERROR = 2,
	LOG_INFO = 3,
	LOG_DEBUG = 4,
	LOG_TRACE = 5
};

int log_configure(int argc, char *argv[]);

void log_write(int level, const char *subsystem,
	       const char *format, ...);


#if LOG_LEVEL_ENABLED >= 5
#define log_trace(format, ...) \
	log_write(LOG_TRACE, LOG_SUBSYSTEM, format, ##__VA_ARGS__);
#else
#define log_trace(format, ...)
#endif

#if LOG_LEVEL_ENABLED >= 4
#define log_debug(format, ...) \
	log_write(LOG_DEBUG, LOG_SUBSYSTEM, format, ##__VA_ARGS__);
#else
#define log_debug(format, ...)
#endif

#if LOG_LEVEL_ENABLED >= 3
#define log_info(format, ...) \
	log_write(LOG_INFO, LOG_SUBSYSTEM, format, ##__VA_ARGS__);
#else
#define log_info(format, ...)
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

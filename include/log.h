#ifndef _WOB_LOG_H
#define _WOB_LOG_H

#include <stdbool.h>

typedef enum {
	WOB_LOG_DEBUG = 0,
	WOB_LOG_INFO = 1,
	WOB_LOG_WARN = 2,
	WOB_LOG_ERROR = 3,
} wob_log_importance;

void wob_log(wob_log_importance importance, const char *file, int line, const char *fmt, ...);

void wob_log_set_level(wob_log_importance importance);

void wob_log_inc_verbosity(void);

void wob_log_use_colors(bool use_colors);

#define wob_log_debug(...) wob_log(WOB_LOG_DEBUG, WOB_FILE, __LINE__, __VA_ARGS__)
#define wob_log_info(...) wob_log(WOB_LOG_INFO, WOB_FILE, __LINE__, __VA_ARGS__)
#define wob_log_warn(...) wob_log(WOB_LOG_WARN, WOB_FILE, __LINE__, __VA_ARGS__)
#define wob_log_error(...) wob_log(WOB_LOG_ERROR, WOB_FILE, __LINE__, __VA_ARGS__)

#define wob_log_level_debug() wob_log_set_level(WOB_LOG_DEBUG);
#define wob_log_level_info() wob_log_set_level(WOB_LOG_INFO);
#define wob_log_level_warn() wob_log_set_level(WOB_LOG_WARN);
#define wob_log_level_error() wob_log_set_level(WOB_LOG_ERROR);

#endif

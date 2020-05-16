#ifndef _WOB_LOG_H
#define _WOB_LOG_H

#include <stdbool.h>

typedef enum {
	_WOB_LOG_DEBUG = 0,
	_WOB_LOG_INFO = 1,
	_WOB_LOG_WARN = 2,
	_WOB_LOG_ERROR = 3,
} _wob_log_importance;

void _wob_log(const _wob_log_importance importance, const char *file, const int line, const char *fmt, ...);

void _wob_log_set_level(const _wob_log_importance importance);

void wob_log_inc_verbosity(void);

void wob_log_use_colors(bool use_colors);

#define wob_log_debug(...) _wob_log(_WOB_LOG_DEBUG, WOB_FILE, __LINE__, __VA_ARGS__)
#define wob_log_info(...) _wob_log(_WOB_LOG_INFO, WOB_FILE, __LINE__, __VA_ARGS__)
#define wob_log_warn(...) _wob_log(_WOB_LOG_WARN, WOB_FILE, __LINE__, __VA_ARGS__)
#define wob_log_error(...) _wob_log(_WOB_LOG_ERROR, WOB_FILE, __LINE__, __VA_ARGS__)

#define wob_log_level_debug() _wob_log_set_level(_WOB_LOG_DEBUG);
#define wob_log_level_info() _wob_log_set_level(_WOB_LOG_INFO);
#define wob_log_level_warn() _wob_log_set_level(_WOB_LOG_WARN);
#define wob_log_level_error() _wob_log_set_level(_WOB_LOG_ERROR);

#endif

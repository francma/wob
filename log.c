#define WOB_FILE "log.c"

#define _POSIX_C_SOURCE 199506L

#define COLOR_RESET "\x1B[0m"
#define COLOR_WHITE "\x1B[1;37m"
#define COLOR_BLACK "\x1B[0;30m"
#define COLOR_BLUE "\x1B[0;34m"
#define COLOR_LIGHT_BLUE "\x1B[1;34m"
#define COLOR_GREEN "\x1B[0;32m"
#define COLOR_LIGHT_GREEN "\x1B[1;32m"
#define COLOR_CYAN "\x1B[0;36m"
#define COLOR_LIGHT_CYAN "\x1B[1;36m"
#define COLOR_RED "\x1B[0;31m"
#define COLOR_LIGHT_RED "\x1B[1;31m"
#define COLOR_PURPLE "\x1B[0;35m"
#define COLOR_LIGHT_PURPLE "\x1B[1;35m"
#define COLOR_BROWN "\x1B[0;33m"
#define COLOR_YELLOW "\x1B[1;33m"
#define COLOR_GRAY "\x1B[0;30m"
#define COLOR_LIGHT_GRAY "\x1B[0;37m"

#include <errno.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "log.h"

static wob_log_importance min_importance_to_log = WOB_LOG_WARN;

static bool use_colors = false;

static const char *verbosity_names[] = {
	"DEBUG",
	"INFO",
	"WARN",
	"ERROR",
};

static const char *verbosity_colors[] = {
	COLOR_LIGHT_CYAN,
	COLOR_GREEN,
	COLOR_YELLOW,
	COLOR_LIGHT_RED,
};

void
wob_log(const wob_log_importance importance, const char *file, const int line, const char *fmt, ...)
{
	if (importance < min_importance_to_log) {
		return;
	}

	struct timespec ts;
	if (clock_gettime(CLOCK_REALTIME, &ts) != 0) {
		fprintf(stderr, "clock_gettime() failed: %s\n", strerror(errno));
		ts.tv_sec = 0;
		ts.tv_nsec = 0;
	}

	// formatting time via localtime() requires open syscall (to read /etc/localtime)
	// and that is problematic with seccomp rules in place
	if (use_colors) {
		fprintf(
			stderr,
			"%jd.%06ld %s%-5s%s %s%s:%d:%s ",
			(intmax_t) ts.tv_sec,
			ts.tv_nsec / 1000,
			verbosity_colors[importance],
			verbosity_names[importance],
			COLOR_RESET,
			COLOR_LIGHT_GRAY,
			file,
			line,
			COLOR_RESET);
	}
	else {
		fprintf(stderr, "%jd.%06ld %s %s:%d: ", (intmax_t) ts.tv_sec, ts.tv_nsec / 1000, verbosity_names[importance], file, line);
	}

	va_list args;
	va_start(args, fmt);
	vfprintf(stderr, fmt, args);
	va_end(args);
	fprintf(stderr, "\n");
}

void
wob_log_set_level(const wob_log_importance importance)
{
	min_importance_to_log = importance;
}

void
wob_log_use_colors(const bool colors)
{
	use_colors = colors;
}

void
wob_log_inc_verbosity(void)
{
	if (min_importance_to_log != WOB_LOG_DEBUG) {
		min_importance_to_log -= 1;
		wob_log_debug("Set log level to %s", verbosity_names[min_importance_to_log]);
	}
}

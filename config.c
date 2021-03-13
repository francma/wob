#define WOB_FILE "config.c"

#include <errno.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "config.h"
#include "log.h"
#include "parse.h"
#include <limits.h>

#include "wlr-layer-shell-unstable-v1-client-protocol.h"
#include "xdg-output-unstable-v1-client-protocol.h"

bool
wob_parse_geometry_from_environment(char *name, struct wob_geom *geom)
{
	char *strtoul_end;
	char *env_string = getenv(name);

	if (env_string == NULL) {
		return false;
	}

	if (strcmp(name, "WOB_ANCHOR") == 0) {
		if (!wob_parse_anchor(env_string, &(geom->anchor))) {
			wob_log_error("Anchor must be one of 'top', 'bottom', 'left', 'right', 'center'.");
			return false;
		}
		return true; // succesfully parsed anchor
	}

	unsigned long value = strtoul(env_string, &strtoul_end, 10);

	if (*strtoul_end != '\0' || errno == ERANGE) {
		wob_log_error("Got invalid value for %s from environment: %s)", name, env_string);
		return false;
	}

	if (strcmp(name, "WOB_WIDTH") == 0) {
		geom->width = value;
	}
	else if (strcmp(name, "WOB_HEIGHT") == 0) {
		geom->height = value;
	}
	else if (strcmp(name, "WOB_BORDER_OFFSET") == 0) {
		geom->border_offset = value;
	}
	else if (strcmp(name, "WOB_BORDER_SIZE") == 0) {
		geom->border_size = value;
	}
	else if (strcmp(name, "WOB_BAR_PADDING") == 0) {
		geom->bar_padding = value;
	}
	else if (strcmp(name, "WOB_STRIDE") == 0) {
		geom->stride = value;
	}
	else if (strcmp(name, "WOB_SIZE") == 0) {
		geom->size = value;
	}
	else if (strcmp(name, "WOB_MARGIN") == 0) {
		geom->margin = value;
	}
	else {
		wob_log_error("Invalid value: %s", name);
		return false;
	}
	return true;
};

bool
wob_parse_config_from_environment(struct wob_geom *geom, struct wob_colors *colors, unsigned long *timeout_msec)
{
	struct environment_config {
		char *name;
		unsigned long *value;
	};
	char *geom_config_names[] = {
		"WOB_WIDTH",
		"WOB_HEIGHT",
		"WOB_BORDER_OFFSET",
		"WOB_BORDER_SIZE",
		"WOB_BAR_PADDING",
		"WOB_STRIDE",
		"WOB_SIZE",
		"WOB_ANCHOR",
		"WOB_MARGIN",
	};
	for (size_t i = 0; i < sizeof(geom_config_names) / sizeof(char *); ++i) {
		char *name = geom_config_names[i];
		if (wob_parse_geometry_from_environment(name, geom)) {
			wob_log_info("Parsed %s from environment", name);
		}
	}

	{
		char *str_end;
		struct wob_color parsed_color = {0};
		char *env_str = getenv("WOB_BAR_COLOR");
		if (env_str && wob_parse_color(env_str, &str_end, &parsed_color)) {
			colors->bar = parsed_color;
		}
	}
	{
		char *str_end;
		struct wob_color parsed_color = {0};
		char *env_str = getenv("WOB_BORDER_COLOR");
		if (env_str && wob_parse_color(env_str, &str_end, &parsed_color)) {
			colors->border = parsed_color;
		}
	}
	{
		char *str_end;
		struct wob_color parsed_color = {0};
		char *env_str = getenv("WOB_BACKGROUND_COLOR");
		if (env_str && wob_parse_color(env_str, &str_end, &parsed_color)) {
			colors->background = parsed_color;
		}
	}

	char *timeout_env = getenv("WOB_TIMEOUT");
	if (timeout_env != NULL && strcmp(timeout_env, "0") != 0) {
		char *strtoul_end;
		unsigned long timeout_msec = strtoul(timeout_env, &strtoul_end, 10);
		if (*strtoul_end != '\0' || errno == ERANGE || timeout_msec == 0) {
			wob_log_error("Timeout must be a value between 1 and %lu.", ULONG_MAX);
			return false;
		}
	}
	return true;
}

#define WOB_FILE "config.c"

#define MIN_PERCENTAGE_BAR_WIDTH 1
#define MIN_PERCENTAGE_BAR_HEIGHT 1

#define _POSIX_C_SOURCE 200809L
#include <ini.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <wordexp.h>

#include "config.h"

bool
parse_output_mode(const char *str, enum wob_output_mode *value)
{
	if (strcmp(str, "whitelist") == 0) {
		*value = WOB_OUTPUT_MODE_WHITELIST;
		return true;
	}

	if (strcmp(str, "all") == 0) {
		*value = WOB_OUTPUT_MODE_ALL;
		return true;
	}

	if (strcmp(str, "focused") == 0) {
		*value = WOB_OUTPUT_MODE_FOCUSED;
		return true;
	}

	return false;
}

bool
parse_overflow_mode(const char *str, enum wob_overflow_mode *value)
{
	if (strcmp(str, "wrap") == 0) {
		*value = WOB_OVERFLOW_MODE_WRAP;
		return true;
	}

	if (strcmp(str, "nowrap") == 0) {
		*value = WOB_OVERFLOW_MODE_NOWRAP;
		return true;
	}

	return false;
}

bool
parse_number(const char *str, unsigned long *value)
{
	char *str_end;
	unsigned long ul = strtoul(str, &str_end, 10);
	if (*str_end != '\0') {
		return false;
	}

	*value = ul;

	return true;
}

bool
parse_anchor(const char *str, unsigned long *anchor)
{
	char str_dup[INI_MAX_LINE + 1] = {0};
	strncpy(str_dup, str, INI_MAX_LINE);

	char *token = strtok(str_dup, " ");

	*anchor = WOB_ANCHOR_CENTER;
	while (token) {
		if (strcmp(token, "left") == 0) {
			*anchor |= WOB_ANCHOR_LEFT;
		}
		else if (strcmp(token, "right") == 0) {
			*anchor |= WOB_ANCHOR_RIGHT;
		}
		else if (strcmp(token, "top") == 0) {
			*anchor |= WOB_ANCHOR_TOP;
		}
		else if (strcmp(token, "bottom") == 0) {
			*anchor |= WOB_ANCHOR_BOTTOM;
		}
		else if (strcmp(token, "center") != 0) {
			return false;
		}

		token = strtok(NULL, " ");
	}

	return true;
}

bool
parse_color(const char *str, struct wob_color *color)
{
	char *str_end;
	if (wob_color_from_string(str, &str_end, color) == false || *str_end != '\0') {
		return false;
	}

	return true;
}

int
handler(void *user, const char *section, const char *name, const char *value)
{
	struct wob_config *config = (struct wob_config *) user;

	unsigned long ul;
	struct wob_color color;

	if (strcmp(section, "") == 0) {
		if (strcmp(name, "max") == 0) {
			if (parse_number(value, &ul) == false || ul < 1 || ul > 10000) {
				wob_log_error("Maximum must be a value between 1 and %lu.", 10000);
				return 0;
			}
			config->max = ul;
			return 1;
		}
		if (strcmp(name, "timeout") == 0) {
			if (parse_number(value, &ul) == false || ul < 1 || ul > 10000) {
				wob_log_error("Timeout must be a value between 1 and %lu.", 10000);
				return 0;
			}
			config->timeout_msec = ul;
			return 1;
		}
		if (strcmp(name, "width") == 0) {
			if (parse_number(value, &ul) == false) {
				wob_log_error("Width must be a positive value.");
				return 0;
			}
			config->dimensions.width = ul;
			return 1;
		}
		if (strcmp(name, "height") == 0) {
			if (parse_number(value, &ul) == false) {
				wob_log_error("Height must be a positive value.");
				return 0;
			}
			config->dimensions.height = ul;
			return 1;
		}
		if (strcmp(name, "border_offset") == 0) {
			if (parse_number(value, &ul) == false) {
				wob_log_error("Border offset must be a positive value.");
				return 0;
			}
			config->dimensions.border_offset = ul;
			return 1;
		}
		if (strcmp(name, "border_size") == 0) {
			if (parse_number(value, &ul) == false) {
				wob_log_error("Border size must be a positive value.");
				return 0;
			}
			config->dimensions.border_size = ul;
			return 1;
		}
		if (strcmp(name, "bar_padding") == 0) {
			printf("%s\n", value);
			if (parse_number(value, &ul) == false) {
				wob_log_error("Bar padding must be a positive value.");
				return 0;
			}
			config->dimensions.bar_padding = ul;
			return 1;
		}
		if (strcmp(name, "margin") == 0) {
			if (parse_number(value, &ul) == false) {
				wob_log_error("Anchor margin must be a positive value.");
				return 0;
			}
			config->margin = ul;
			return 1;
		}
		if (strcmp(name, "anchor") == 0) {
			if (parse_anchor(value, &ul) == false) {
				wob_log_error("Anchor must be one of 'top', 'bottom', 'left', 'right', 'center'.");
				return 0;
			}
			config->anchor = ul;
			return 1;
		}
		if (strcmp(name, "background_color") == 0) {
			if (!parse_color(value, &color)) {
				wob_log_error("Background color must be in RRGGBB[AA] format");
				return 0;
			}
			config->colors.background = color;
			return 1;
		}
		if (strcmp(name, "border_color") == 0) {
			if (!parse_color(value, &color)) {
				wob_log_error("Border color must be in RRGGBB[AA] format.");
				return 0;
			}
			config->colors.border = color;
			return 1;
		}
		if (strcmp(name, "bar_color") == 0) {
			if (!parse_color(value, &color)) {
				wob_log_error("Bar color must be in RRGGBB[AA] format.");
				return 0;
			}
			config->colors.value = color;
			return 1;
		}
		if (strcmp(name, "overflow_background_color") == 0) {
			if (!parse_color(value, &color)) {
				wob_log_error("Overflow background color must be in RRGGBB[AA] format.");
				return 0;
			}
			config->overflow_colors.background = color;
			return 1;
		}
		if (strcmp(name, "overflow_border_color") == 0) {
			if (!parse_color(value, &color)) {
				wob_log_error("Overflow border color must be in RRGGBB[AA] format.");
				return 0;
			}
			config->overflow_colors.border = color;
			return 1;
		}
		if (strcmp(name, "overflow_bar_color") == 0) {
			if (!parse_color(value, &color)) {
				wob_log_error("Overflow bar color must be in RRGGBB[AA] format.");
				return 0;
			}
			config->overflow_colors.value = color;
			return 1;
		}
		if (strcmp(name, "overflow_mode") == 0) {
			if (parse_overflow_mode(value, &config->overflow_mode) == false) {
				wob_log_error("Invalid argument for overflow-mode. Valid options are wrap and nowrap.");
				return 0;
			}
			return 1;
		}
		if (strcmp(name, "output_mode") == 0) {
			if (parse_output_mode(value, &config->output_mode) == false) {
				wob_log_error("Invalid argument for output_mode. Valid options are focused, whitelist and all.");
				return 0;
			}
			return 1;
		}

		wob_log_error("Unknown config key %s", name);
		return 0;
	}

	if (strncmp(section, "output.", sizeof("output.") - 1) == 0) {
		if (strcmp(name, "name") == 0) {
			struct wob_output_config *output_config = malloc(sizeof(struct wob_output_config));
			output_config->name = strdup(value);

			wl_list_insert(&config->outputs, &output_config->link);
			return 1;
		}

		wob_log_error("Unknown config key %s", name);
		return 0;
	}

	wob_log_error("Unknown config section %s", section);
	return 0;
}

char *
wob_config_default_path()
{
	static const char *config_paths[] = {
		"$XDG_CONFIG_HOME/wob/wob.ini",
		"$HOME/.config/wob/wob.ini",
	};

	for (size_t i = 0; i < sizeof(config_paths) / sizeof(char *); ++i) {
		wob_log_debug("Looking for config file at %s", config_paths[i]);

		wordexp_t p;
		if (wordexp(config_paths[i], &p, WRDE_UNDEF) == 0) {
			char *path = strdup(p.we_wordv[0]);
			wordfree(&p);

			if (access(path, F_OK) == 0) {
				wob_log_info("Found configuration file at %s", config_paths[i]);
				return path;
			}

			wob_log_debug("Configuration file at %s not found", config_paths[i]);
			free(path);
		}
	}

	return NULL;
}

void
wob_config_init(struct wob_config *config)
{
	wl_list_init(&config->outputs);

	config->max = 100;
	config->timeout_msec = 1000;
	config->dimensions.width = 400;
	config->dimensions.height = 50;
	config->dimensions.border_offset = 4;
	config->dimensions.border_size = 4;
	config->dimensions.bar_padding = 4;
	config->margin = 0;
	config->anchor = WOB_ANCHOR_CENTER;
	config->overflow_mode = WOB_OVERFLOW_MODE_WRAP;
	config->output_mode = WOB_OUTPUT_MODE_FOCUSED;
	config->colors.background = (struct wob_color){.a = 1.0f, .r = 0.0f, .g = 0.0f, .b = 0.0f};
	config->colors.value = (struct wob_color){.a = 1.0f, .r = 1.0f, .g = 1.0f, .b = 1.0f};
	config->colors.border = (struct wob_color){.a = 1.0f, .r = 1.0f, .g = 1.0f, .b = 1.0f};
	config->overflow_colors.background = (struct wob_color){.a = 1.0f, .r = 0.0f, .g = 0.0f, .b = 0.0f};
	config->overflow_colors.value = (struct wob_color){.a = 1.0f, .r = 1.0f, .g = 0.0f, .b = 0.0f};
	config->overflow_colors.border = (struct wob_color){.a = 1.0f, .r = 1.0f, .g = 1.0f, .b = 1.0f};
}

bool
wob_config_load(struct wob_config *config, const char *config_path)
{
	int res = ini_parse(config_path, handler, config);

	if (res == -1 || res == -2) {
		wob_log_error("Failed to open config file %s", config_path);

		return false;
	}

	if (res != 0) {
		wob_log_error("Failed to parse config file %s, error at line %d", config_path, res);

		return false;
	}

	struct wob_dimensions dimensions = config->dimensions;
	if (dimensions.width < MIN_PERCENTAGE_BAR_WIDTH + 2 * (dimensions.border_offset + dimensions.border_size + dimensions.bar_padding)) {
		wob_log_error("Invalid geometry: width is too small for given parameters");
		return false;
	}

	if (dimensions.height < MIN_PERCENTAGE_BAR_HEIGHT + 2 * (dimensions.border_offset + dimensions.border_size + dimensions.bar_padding)) {
		wob_log_error("Invalid geometry: height is too small for given parameters");
		return false;
	}

	return true;
}

void
wob_config_debug(struct wob_config *config)
{
	wob_log_debug("config.max = %lu", config->max);
	wob_log_debug("config.timeout_msec = %lu", config->timeout_msec);
	wob_log_debug("config.dimensions.width = %lu", config->dimensions.width);
	wob_log_debug("config.dimensions.height = %lu", config->dimensions.height);
	wob_log_debug("config.dimensions.border_offset = %lu", config->dimensions.border_offset);
	wob_log_debug("config.dimensions.border_size = %lu", config->dimensions.border_size);
	wob_log_debug("config.dimensions.bar_padding = %lu", config->dimensions.bar_padding);
	wob_log_debug("config.margin = %lu", config->margin);
	wob_log_debug("config.anchor = %lu (top = %d, bottom = %d, left = %d, right = %d)", config->anchor, WOB_ANCHOR_TOP, WOB_ANCHOR_BOTTOM, WOB_ANCHOR_LEFT, WOB_ANCHOR_RIGHT);
	wob_log_debug("config.overflow_mode = %lu (wrap = %d, nowrap = %d)", config->overflow_mode, WOB_OVERFLOW_MODE_WRAP, WOB_OVERFLOW_MODE_NOWRAP);
	wob_log_debug(
		"config.output_mode = %lu (whitelist = %d, all = %d, focused = %d)", config->output_mode, WOB_OUTPUT_MODE_WHITELIST, WOB_OUTPUT_MODE_ALL, WOB_OUTPUT_MODE_FOCUSED);
	wob_log_debug("config.colors.background = #%08jx", (uintmax_t) wob_color_to_rgba(config->colors.background));
	wob_log_debug("config.colors.value = #%08jx", (uintmax_t) wob_color_to_rgba(config->colors.value));
	wob_log_debug("config.colors.border = #%08jx", (uintmax_t) wob_color_to_rgba(config->colors.border));
	wob_log_debug("config.overflow_colors.background = #%08jx", (uintmax_t) wob_color_to_rgba(config->overflow_colors.background));
	wob_log_debug("config.overflow_colors.value = #%08jx", (uintmax_t) wob_color_to_rgba(config->overflow_colors.value));
	wob_log_debug("config.overflow_colors.border = #%08jx", (uintmax_t) wob_color_to_rgba(config->overflow_colors.border));

	struct wob_output_config *output_config;
	wl_list_for_each (output_config, &config->outputs, link) {
		wob_log_debug("config.output.%s.name = %s", output_config->name, output_config->name);
	}
}

void
wob_config_destroy(struct wob_config *config)
{
	struct wob_output_config *output, *output_tmp;
	wl_list_for_each_safe (output, output_tmp, &config->outputs, link) {
		free(output->name);
		free(output);
	}
}

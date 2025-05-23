#define WOB_FILE "config.c"

#define MIN_PERCENTAGE_BAR_WIDTH 1
#define MIN_PERCENTAGE_BAR_HEIGHT 1

#include <ini.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <wordexp.h>

#include "config.h"

#include "global_configuration.h"
#include "log.h"

bool
parse_margin(const char *str, struct wob_margin *margin)
{
	char str_dup[INI_MAX_LINE + 1] = {0};
	strncpy(str_dup, str, INI_MAX_LINE);

	size_t i = 0;
	unsigned long values[4] = {0};

	char *token = strtok(str_dup, " ");
	while (token) {
		if (i >= 4) {
			return false;
		}

		char *str_end;
		unsigned long ul = strtoul(token, &str_end, 10);
		if (*str_end != '\0') {
			return false;
		}

		values[i] = ul;

		token = strtok(NULL, " ");
		i += 1;
	}

	switch (i) {
		case 4:
			margin->top = values[0];
			margin->right = values[1];
			margin->bottom = values[2];
			margin->left = values[3];
			break;
		case 1:
			margin->top = values[0];
			margin->right = values[0];
			margin->bottom = values[0];
			margin->left = values[0];
			break;
		default:
			return false;
	}

	return true;
}

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
	return wob_color_from_rgba_string(str, color);
}

bool
parse_orientation(const char *str, enum wob_orientation *value)
{
	if (strcmp(str, "horizontal") == 0) {
		*value = WOB_ORIENTATION_HORIZONTAL;
		return true;
	}

	if (strcmp(str, "vertical") == 0) {
		*value = WOB_ORIENTATION_VERTICAL;
		return true;
	}

	return false;
}

int
handler(void *user, const char *section, const char *name, const char *value)
{
	struct wob_config *config = user;

	unsigned long ul;

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
			if (parse_number(value, &ul) == false) {
				wob_log_error("Bar padding must be a positive value.");
				return 0;
			}
			config->dimensions.bar_padding = ul;
			return 1;
		}
		if (strcmp(name, "margin") == 0) {
			if (parse_margin(value, &config->margin) == false) {
				wob_log_error("Margin must be in format <value> or <value_top> <value_right> <value_bottom> <value_left>.");
				return 0;
			}
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
			if (!parse_color(value, &config->default_style.colors.background)) {
				wob_log_error("Background color must be in RRGGBB[AA] format");
				return 0;
			}
			return 1;
		}
		if (strcmp(name, "border_color") == 0) {
			if (!parse_color(value, &config->default_style.colors.border)) {
				wob_log_error("Border color must be in RRGGBB[AA] format.");
				return 0;
			}
			return 1;
		}
		if (strcmp(name, "bar_color") == 0) {
			if (!parse_color(value, &config->default_style.colors.value)) {
				wob_log_error("Bar color must be in RRGGBB[AA] format.");
				return 0;
			}
			return 1;
		}
		if (strcmp(name, "overflow_background_color") == 0) {
			if (!parse_color(value, &config->default_style.overflow_colors.background)) {
				wob_log_error("Overflow background color must be in RRGGBB[AA] format.");
				return 0;
			}
			return 1;
		}
		if (strcmp(name, "overflow_border_color") == 0) {
			if (!parse_color(value, &config->default_style.overflow_colors.border)) {
				wob_log_error("Overflow border color must be in RRGGBB[AA] format.");
				return 0;
			}
			return 1;
		}
		if (strcmp(name, "overflow_bar_color") == 0) {
			if (!parse_color(value, &config->default_style.overflow_colors.value)) {
				wob_log_error("Overflow bar color must be in RRGGBB[AA] format.");
				return 0;
			}
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
			wob_log_warn("output_mode was removed, now it always behaves as \"focused\"");
			return 0;
		}
		if (strcmp(name, "orientation") == 0) {
			if (parse_orientation(value, &config->dimensions.orientation) == false) {
				wob_log_error("Invalid argument for orientation. Valid options are horizontal and vertical");
				return 0;
			}
			return 1;
		}

		wob_log_warn("Unknown config key %s", name);
		return 1;
	}

	if (strncmp(section, "output.", sizeof("output.") - 1) == 0) {
		char output_id[INI_MAX_LINE + 1] = {0};
		strncpy(output_id, section + sizeof("output.") - 1, INI_MAX_LINE);

		struct wob_output_config *output_config = wob_config_find_output(config, output_id);
		if (output_config == NULL) {
			output_config = calloc(1, sizeof(struct wob_output_config));
			if (output_config == NULL) {
				wob_log_panic("calloc() failed");
			}

			output_config->id = strdup(output_id);
			output_config->match = NULL;
			output_config->dimensions = config->dimensions;
			output_config->margin = config->margin;
			output_config->anchor = config->anchor;
			wl_list_insert(&config->outputs, &output_config->link);
		}

		if (strcmp(name, "match") == 0) {
			output_config->match = strdup(value);
			return 1;
		}
		if (strcmp(name, "width") == 0) {
			if (parse_number(value, &ul) == false) {
				wob_log_error("Width must be a positive value.");
				return 0;
			}
			output_config->dimensions.width = ul;
			return 1;
		}
		if (strcmp(name, "height") == 0) {
			if (parse_number(value, &ul) == false) {
				wob_log_error("Height must be a positive value.");
				return 0;
			}
			output_config->dimensions.height = ul;
			return 1;
		}
		if (strcmp(name, "margin") == 0) {
			if (parse_margin(value, &output_config->margin) == false) {
				wob_log_error("Margin must be in format <value> or <value_top> <value_right> <value_bottom> <value_left>.");
				return 0;
			}
			return 1;
		}
		if (strcmp(name, "anchor") == 0) {
			if (parse_anchor(value, &ul) == false) {
				wob_log_error("Anchor must be one of 'top', 'bottom', 'left', 'right', 'center'.");
				return 0;
			}
			output_config->anchor = ul;
			return 1;
		}
		if (strcmp(name, "border_offset") == 0) {
			if (parse_number(value, &ul) == false) {
				wob_log_error("Border offset must be a positive value.");
				return 0;
			}
			output_config->dimensions.border_offset = ul;
			return 1;
		}
		if (strcmp(name, "border_size") == 0) {
			if (parse_number(value, &ul) == false) {
				wob_log_error("Border size must be a positive value.");
				return 0;
			}
			output_config->dimensions.border_size = ul;
			return 1;
		}
		if (strcmp(name, "bar_padding") == 0) {
			if (parse_number(value, &ul) == false) {
				wob_log_error("Bar padding must be a positive value.");
				return 0;
			}
			output_config->dimensions.bar_padding = ul;
			return 1;
		}

		wob_log_warn("Unknown config key %s", name);
		return 1;
	}

	if (strncmp(section, "style.", sizeof("style.") - 1) == 0) {
		char style_name[INI_MAX_LINE + 1] = {0};
		strncpy(style_name, section + sizeof("style.") - 1, INI_MAX_LINE);

		struct wob_style *style = wob_config_find_style(config, style_name);
		if (style == NULL) {
			style = calloc(1, sizeof(struct wob_style));
			if (style == NULL) {
				wob_log_panic("calloc() failed");
			}

			style->name = strdup(style_name);
			style->colors = config->default_style.colors;
			style->overflow_colors = config->default_style.overflow_colors;
			wl_list_insert(&config->styles, &style->link);
		}

		if (strcmp(name, "background_color") == 0) {
			if (!parse_color(value, &style->colors.background)) {
				wob_log_error("Background color must be in RRGGBB[AA] format");
				return 0;
			}
			return 1;
		}
		if (strcmp(name, "border_color") == 0) {
			if (!parse_color(value, &style->colors.border)) {
				wob_log_error("Border color must be in RRGGBB[AA] format.");
				return 0;
			}
			return 1;
		}
		if (strcmp(name, "bar_color") == 0) {
			if (!parse_color(value, &style->colors.value)) {
				wob_log_error("Bar color must be in RRGGBB[AA] format.");
				return 0;
			}
			return 1;
		}
		if (strcmp(name, "overflow_background_color") == 0) {
			if (!parse_color(value, &style->overflow_colors.background)) {
				wob_log_error("Overflow background color must be in RRGGBB[AA] format.");
				return 0;
			}
			return 1;
		}
		if (strcmp(name, "overflow_border_color") == 0) {
			if (!parse_color(value, &style->overflow_colors.border)) {
				wob_log_error("Overflow border color must be in RRGGBB[AA] format.");
				return 0;
			}
			return 1;
		}
		if (strcmp(name, "overflow_bar_color") == 0) {
			if (!parse_color(value, &style->overflow_colors.value)) {
				wob_log_error("Overflow bar color must be in RRGGBB[AA] format.");
				return 0;
			}
			return 1;
		}

		wob_log_warn("Unknown config key %s", name);
		return 1;
	}

	wob_log_warn("Unknown config section %s", section);
	return 1;
}

char *
wob_config_default_path()
{
	static const char *config_paths[] = {
		"$XDG_CONFIG_HOME/wob/wob.ini",
		"$HOME/.config/wob/wob.ini",
		WOB_ETC_CONFIG_FOLDER_PATH "/wob.ini",
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

struct wob_config *
wob_config_create()
{
	struct wob_config *config = malloc(sizeof(struct wob_config));

	wl_list_init(&config->outputs);
	wl_list_init(&config->styles);

	config->sandbox = true;
	config->max = 100;
	config->timeout_msec = 1000;
	config->dimensions.width = 400;
	config->dimensions.height = 50;
	config->dimensions.border_offset = 4;
	config->dimensions.border_size = 4;
	config->dimensions.bar_padding = 4;
	config->dimensions.orientation = WOB_ORIENTATION_HORIZONTAL;
	config->margin = (struct wob_margin) {.top = 0, .left = 0, .bottom = 0, .right = 0};
	config->anchor = WOB_ANCHOR_CENTER;
	config->overflow_mode = WOB_OVERFLOW_MODE_WRAP;
	config->default_style.colors.background = (struct wob_color) {.a = 1.0f, .r = 0.0f, .g = 0.0f, .b = 0.0f};
	config->default_style.colors.value = (struct wob_color) {.a = 1.0f, .r = 1.0f, .g = 1.0f, .b = 1.0f};
	config->default_style.colors.border = (struct wob_color) {.a = 1.0f, .r = 1.0f, .g = 1.0f, .b = 1.0f};
	config->default_style.overflow_colors.background = (struct wob_color) {.a = 1.0f, .r = 0.0f, .g = 0.0f, .b = 0.0f};
	config->default_style.overflow_colors.value = (struct wob_color) {.a = 1.0f, .r = 1.0f, .g = 0.0f, .b = 0.0f};
	config->default_style.overflow_colors.border = (struct wob_color) {.a = 1.0f, .r = 1.0f, .g = 1.0f, .b = 1.0f};

	return config;
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

	struct wob_output_config *output;
	wl_list_for_each (output, &config->outputs, link) {
		if (output->match == NULL) {
			wob_log_error("Output %s is missing \"match\" property", output->id);
			return false;
		}
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
	wob_log_debug("config.dimensions.orientation = %lu (horizontal = %d, vertical = %d)", config->dimensions.orientation, WOB_ORIENTATION_HORIZONTAL, WOB_ORIENTATION_VERTICAL);
	wob_log_debug("config.margin.top = %lu", config->margin.top);
	wob_log_debug("config.margin.right = %lu", config->margin.right);
	wob_log_debug("config.margin.bottom = %lu", config->margin.bottom);
	wob_log_debug("config.margin.left = %lu", config->margin.left);
	wob_log_debug("config.anchor = %lu (top = %d, bottom = %d, left = %d, right = %d)", config->anchor, WOB_ANCHOR_TOP, WOB_ANCHOR_BOTTOM, WOB_ANCHOR_LEFT, WOB_ANCHOR_RIGHT);
	wob_log_debug("config.overflow_mode = %lu (wrap = %d, nowrap = %d)", config->overflow_mode, WOB_OVERFLOW_MODE_WRAP, WOB_OVERFLOW_MODE_NOWRAP);

	wob_log_debug("config.colors.background = " WOB_COLOR_PRINTF_FORMAT, WOB_COLOR_PRINTF_RGBA(config->default_style.colors.background));
	wob_log_debug("config.colors.value = " WOB_COLOR_PRINTF_FORMAT, WOB_COLOR_PRINTF_RGBA(config->default_style.colors.value));
	wob_log_debug("config.colors.border = " WOB_COLOR_PRINTF_FORMAT, WOB_COLOR_PRINTF_RGBA(config->default_style.colors.border));
	wob_log_debug("config.overflow_colors.background = " WOB_COLOR_PRINTF_FORMAT, WOB_COLOR_PRINTF_RGBA(config->default_style.overflow_colors.background));
	wob_log_debug("config.overflow_colors.value = " WOB_COLOR_PRINTF_FORMAT, WOB_COLOR_PRINTF_RGBA(config->default_style.overflow_colors.value));
	wob_log_debug("config.overflow_colors.border = " WOB_COLOR_PRINTF_FORMAT, WOB_COLOR_PRINTF_RGBA(config->default_style.overflow_colors.border));

	struct wob_style *style;
	wl_list_for_each (style, &config->styles, link) {
		wob_log_debug("config.style.%s.colors.background = " WOB_COLOR_PRINTF_FORMAT, style->name, WOB_COLOR_PRINTF_RGBA(style->colors.background));
		wob_log_debug("config.style.%s.colors.value = " WOB_COLOR_PRINTF_FORMAT, style->name, WOB_COLOR_PRINTF_RGBA(style->colors.value));
		wob_log_debug("config.style.%s.colors.border = " WOB_COLOR_PRINTF_FORMAT, style->name, WOB_COLOR_PRINTF_RGBA(style->colors.border));
		wob_log_debug("config.style.%s.overflow_colors.background = " WOB_COLOR_PRINTF_FORMAT, style->name, WOB_COLOR_PRINTF_RGBA(style->overflow_colors.background));
		wob_log_debug("config.style.%s.overflow_colors.value = " WOB_COLOR_PRINTF_FORMAT, style->name, WOB_COLOR_PRINTF_RGBA(style->overflow_colors.value));
		wob_log_debug("config.style.%s.overflow_colors.border = " WOB_COLOR_PRINTF_FORMAT, style->name, WOB_COLOR_PRINTF_RGBA(style->overflow_colors.border));
	}

	struct wob_output_config *output_config;
	wl_list_for_each (output_config, &config->outputs, link) {
		wob_log_debug("config.output.%s.match = %s", output_config->id, output_config->match);
		wob_log_debug("config.output.%s.margin.top = %lu", output_config->id, output_config->margin.top);
		wob_log_debug("config.output.%s.margin.right = %lu", output_config->id, output_config->margin.right);
		wob_log_debug("config.output.%s.margin.bottom = %lu", output_config->id, output_config->margin.bottom);
		wob_log_debug("config.output.%s.margin.left = %lu", output_config->id, output_config->margin.left);
		wob_log_debug("config.output.%s.dimensions.width = %lu", output_config->id, output_config->dimensions.width);
		wob_log_debug("config.output.%s.dimensions.height = %lu", output_config->id, output_config->dimensions.height);
		wob_log_debug("config.output.%s.dimensions.border_offset = %lu", output_config->id, output_config->dimensions.border_offset);
		wob_log_debug("config.output.%s.dimensions.border_size = %lu", output_config->id, output_config->dimensions.border_size);
		wob_log_debug("config.output.%s.dimensions.bar_padding = %lu", output_config->id, output_config->dimensions.bar_padding);
		wob_log_debug(
			"config.output.%s.dimensions.orientation = %lu (horizontal = %d, vertical = %d)",
			output_config->id,
			output_config->dimensions.orientation,
			WOB_ORIENTATION_HORIZONTAL,
			WOB_ORIENTATION_VERTICAL
		);
	}
}

void
wob_config_destroy(struct wob_config *config)
{
	struct wob_output_config *output, *output_tmp;
	wl_list_for_each_safe (output, output_tmp, &config->outputs, link) {
		free(output->match);
		free(output->id);
		free(output);
	}

	struct wob_style *style, *style_tmp;
	wl_list_for_each_safe (style, style_tmp, &config->styles, link) {
		free(style->name);
		free(style);
	}

	free(config);
}

struct wob_style *
wob_config_find_style(struct wob_config *config, const char *style_name)
{
	struct wob_style *style = NULL;
	bool style_found = false;
	wl_list_for_each (style, &config->styles, link) {
		if (strcmp(style->name, style_name) == 0) {
			style_found = true;
			break;
		}
	}

	if (style_found) {
		return style;
	}
	else {
		return NULL;
	}
}

struct wob_output_config *
wob_config_find_output(struct wob_config *config, const char *output_id)
{
	struct wob_output_config *output_config = NULL;
	bool output_found = false;
	wl_list_for_each (output_config, &config->outputs, link) {
		if (strcmp(output_config->id, output_id) == 0) {
			output_found = true;
			break;
		}
	}

	if (output_found) {
		return output_config;
	}
	else {
		return NULL;
	}
}

struct wob_output_config *
wob_config_match_output(struct wob_config *config, const char *match)
{
	struct wob_output_config *output_config = NULL;
	bool output_found = false;
	wl_list_for_each (output_config, &config->outputs, link) {
		if (strstr(output_config->match, match) == 0) {
			output_found = true;
			break;
		}
	}

	if (output_found) {
		return output_config;
	}
	else {
		return NULL;
	}
}

uint32_t
scale_apply(uint32_t base, uint32_t scale)
{
	return lround(base * (scale / 120.));
}

struct wob_dimensions
wob_dimensions_apply_scale(struct wob_dimensions dimensions, uint32_t scale)
{
	struct wob_dimensions scaled_dimensions = {
		.width = scale_apply(dimensions.width, scale),
		.height = scale_apply(dimensions.height, scale),
		.bar_padding = scale_apply(dimensions.bar_padding, scale),
		.border_offset = scale_apply(dimensions.border_offset, scale),
		.border_size = scale_apply(dimensions.border_size, scale),
		.orientation = dimensions.orientation,
	};

	return scaled_dimensions;
}

bool
wob_dimensions_eq(struct wob_dimensions a, struct wob_dimensions b)
{
	if (a.height != b.height) return false;
	if (a.width != b.width) return false;
	if (a.orientation != b.orientation) return false;
	if (a.border_offset != b.border_offset) return false;
	if (a.border_size != b.border_size) return false;
	if (a.bar_padding != b.bar_padding) return false;

	return true;
}

bool
wob_margin_eq(struct wob_margin a, struct wob_margin b)
{
	if (a.top != b.top) return false;
	if (a.right != b.right) return false;
	if (a.bottom != b.bottom) return false;
	if (a.left != b.left) return false;

	return true;
}

#ifndef _WOB_CONFIG_H
#define _WOB_CONFIG_H

#include <stdbool.h>
#include <wayland-util.h>

#include "color.h"
#include "log.h"

enum wob_overflow_mode {
	WOB_OVERFLOW_MODE_WRAP,
	WOB_OVERFLOW_MODE_NOWRAP,
};

enum wob_anchor {
	WOB_ANCHOR_CENTER = 0,
	WOB_ANCHOR_TOP = 1,
	WOB_ANCHOR_BOTTOM = 2,
	WOB_ANCHOR_LEFT = 4,
	WOB_ANCHOR_RIGHT = 8,
};

enum wob_output_mode {
	WOB_OUTPUT_MODE_WHITELIST,
	WOB_OUTPUT_MODE_ALL,
	WOB_OUTPUT_MODE_FOCUSED,
};

struct wob_output_config {
	char *name;
	struct wl_list link;
};

struct wob_colors {
	struct wob_color background;
	struct wob_color border;
	struct wob_color value;
};

struct wob_dimensions {
	unsigned long width;
	unsigned long height;
	unsigned long border_offset;
	unsigned long border_size;
	unsigned long bar_padding;
};

struct wob_config {
	unsigned long max;
	unsigned long timeout_msec;
	unsigned long margin;
	unsigned long anchor;
	enum wob_output_mode output_mode;
	enum wob_overflow_mode overflow_mode;
	struct wob_dimensions dimensions;
	struct wob_colors colors;
	struct wob_colors overflow_colors;
	struct wl_list outputs;
};

void wob_config_init(struct wob_config *config);

bool wob_config_load(struct wob_config *config, const char *config_path);

void wob_config_destroy(struct wob_config *config);

void wob_config_debug(struct wob_config *config);

char *wob_config_default_path();

#endif

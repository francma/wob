#ifndef _WOB_CONFIG_H
#define _WOB_CONFIG_H

#include <stdbool.h>
#include <wayland-util.h>

#include "color.h"

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

enum wob_orientation {
	WOB_ORIENTATION_HORIZONTAL,
	WOB_ORIENTATION_VERTICAL,
};

struct wob_margin {
	unsigned long top;
	unsigned long right;
	unsigned long bottom;
	unsigned long left;
};

struct wob_dimensions {
	unsigned long width;
	unsigned long height;
	unsigned long border_offset;
	unsigned long border_size;
	unsigned long bar_padding;
	enum wob_orientation orientation;
};

struct wob_output_config {
	char *id;
	char *match;
	struct wl_list link;
	struct wob_dimensions dimensions;
	struct wob_margin margin;
	unsigned long anchor;
};

struct wob_colors {
	struct wob_color background;
	struct wob_color border;
	struct wob_color value;
};

struct wob_style {
	char *name;
	struct wob_colors colors;
	struct wob_colors overflow_colors;
	struct wl_list link;
};

struct wob_config {
	unsigned long max;
	unsigned long timeout_msec;
	struct wob_margin margin;
	unsigned long anchor;
	enum wob_overflow_mode overflow_mode;
	struct wob_dimensions dimensions;
	struct wob_style default_style;
	struct wl_list styles;
	struct wl_list outputs;
	bool sandbox;
};

struct wob_config *wob_config_create();

bool wob_config_load(struct wob_config *config, const char *config_path);

void wob_config_destroy(struct wob_config *config);

void wob_config_debug(struct wob_config *config);

char *wob_config_default_path();

struct wob_style *wob_config_find_style(struct wob_config *config, const char *style_name);

struct wob_output_config *wob_config_find_output(struct wob_config *config, const char *output_id);

struct wob_output_config *wob_config_match_output(struct wob_config *config, const char *match);

struct wob_dimensions wob_dimensions_apply_scale(struct wob_dimensions dimensions, uint32_t scale);

bool wob_dimensions_eq(struct wob_dimensions a, struct wob_dimensions b);

bool wob_margin_eq(struct wob_margin a, struct wob_margin b);

#endif

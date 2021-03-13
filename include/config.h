#ifndef _WOB_CONFIG_H
#define _WOB_CONFIG_H

#include <stdbool.h>

#include "color.h"

struct wob_geom {
	unsigned long width;
	unsigned long height;
	unsigned long border_offset;
	unsigned long border_size;
	unsigned long bar_padding;
	unsigned long stride;
	unsigned long size;
	unsigned long anchor;
	unsigned long margin;
};

struct wob_colors {
	struct wob_color bar;
	struct wob_color background;
	struct wob_color border;
};

bool wob_parse_config_from_environment(struct wob_geom *geom, struct wob_colors *colors, unsigned long *timeout_msec);

#endif

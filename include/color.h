#ifndef _WOB_COLOR_H
#define _WOB_COLOR_H

#include <stdbool.h>
#include <stdint.h>

struct wob_color {
	float a;
	float r;
	float g;
	float b;
};

uint32_t wob_color_to_argb(struct wob_color color);

uint32_t wob_color_to_rgba(struct wob_color color);

struct wob_color wob_color_premultiply_alpha(struct wob_color color);

bool wob_color_from_string(const char *restrict str, char **restrict str_end, struct wob_color *color);

#endif

#ifndef _WOB_COLOR_H
#define _WOB_COLOR_H

#include <stdbool.h>
#include <stdint.h>

#define WOB_COLOR_PRINTF_FORMAT "%02X%02X%02X%02X"
#define WOB_COLOR_PRINTF_RGBA(x) (uint8_t) (x.r * UINT8_MAX), (uint8_t) (x.g * UINT8_MAX), (uint8_t) (x.b * UINT8_MAX), (uint8_t) (x.a * UINT8_MAX)

struct wob_color {
	float a;
	float r;
	float g;
	float b;
};

uint32_t wob_color_to_argb(struct wob_color color);

uint32_t wob_color_to_rgba(struct wob_color color);

struct wob_color wob_color_premultiply_alpha(struct wob_color color);

bool wob_color_from_rgba_string(const char *str, struct wob_color *color);

#endif

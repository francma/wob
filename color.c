#define WOB_FILE "color.c"

#include <stdint.h>

#include "color.h"

uint32_t
wob_color_to_argb(const struct wob_color color)
{
	uint8_t alpha = (uint8_t) (color.a * UINT8_MAX);
	uint8_t red = (uint8_t) (color.r * UINT8_MAX);
	uint8_t green = (uint8_t) (color.g * UINT8_MAX);
	uint8_t blue = (uint8_t) (color.b * UINT8_MAX);

	return (alpha << 24) + (red << 16) + (green << 8) + blue;
}

uint32_t
wob_color_to_rgba(const struct wob_color color)
{
	uint8_t alpha = (uint8_t) (color.a * UINT8_MAX);
	uint8_t red = (uint8_t) (color.r * UINT8_MAX);
	uint8_t green = (uint8_t) (color.g * UINT8_MAX);
	uint8_t blue = (uint8_t) (color.b * UINT8_MAX);

	return (red << 24) + (green << 16) + (blue << 8) + alpha;
}

struct wob_color
wob_color_premultiply_alpha(const struct wob_color color)
{
	struct wob_color premultiplied_color = {
		.a = color.a,
		.r = color.r * color.a,
		.g = color.g * color.a,
		.b = color.b * color.a,
	};

	return premultiplied_color;
}

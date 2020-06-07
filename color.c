#define WOB_FILE "color.c"

#include <stdint.h>

#include "color.h"

uint32_t
wob_color_to_argb(const struct wob_color color)
{
	uint8_t alpha = color.alpha * UINT8_MAX;
	uint8_t red = color.red * UINT8_MAX;
	uint8_t green = color.green * UINT8_MAX;
	uint8_t blue = color.blue * UINT8_MAX;

	return (alpha << 24) + (red << 16) + (green << 8) + blue;
}

struct wob_color
wob_color_premultiply_alpha(const struct wob_color color)
{
	struct wob_color premultiplied_color = {
		.alpha = color.alpha,
		.red = color.red * color.alpha,
		.green = color.green * color.alpha,
		.blue = color.blue * color.alpha,
	};

	return premultiplied_color;
}
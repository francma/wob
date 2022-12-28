#define WOB_FILE "color.c"

#include <ctype.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

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

bool
wob_color_from_string(const char *restrict str, char **restrict str_end, struct wob_color *color)
{
	char buffer[3] = {0};
	uint8_t parts[4];
	parts[3] = 0xFF;

	int i;
	for (i = 0; i < 4; ++i) {
		strncpy(buffer, &str[i * 2], 2);
		if (!isxdigit(buffer[0]) || !isxdigit(buffer[1])) {
			break;
		}

		parts[i] = strtoul(buffer, NULL, 16);
	}

	if (i < 3) {
		return false;
	}

	if (str_end) {
		*str_end = ((char *) str) + i * 2;
	}

	*color = (struct wob_color){
		.r = (float) parts[0] / UINT8_MAX,
		.g = (float) parts[1] / UINT8_MAX,
		.b = (float) parts[2] / UINT8_MAX,
		.a = (float) parts[3] / UINT8_MAX,
	};

	return true;
}

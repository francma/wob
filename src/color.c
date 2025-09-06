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

int
hex_to_int(char c)
{
	if (c >= '0' && c <= '9') {
		return c - '0';
	}
	if (c >= 'a' && c <= 'f') {
		return c - 'a' + 10;
	}
	if (c >= 'A' && c <= 'F') {
		return c - 'A' + 10;
	}

	return -1;
}

bool
wob_color_from_rgba_string(const char *str, struct wob_color *color)
{
	unsigned long length = strlen(str);
	int p[8];

	uint8_t parts[4];
	parts[3] = 0xFF;
	switch (length) {
		case 8:
			p[6] = hex_to_int(str[6]);
			p[7] = hex_to_int(str[7]);

			if (p[6] < 0 || p[7] < 0) {
				return false;
			}

			parts[3] = p[6] * 16 + p[7];
			// fallthrough
		case 6:
			p[0] = hex_to_int(str[0]);
			p[1] = hex_to_int(str[1]);
			p[2] = hex_to_int(str[2]);
			p[3] = hex_to_int(str[3]);
			p[4] = hex_to_int(str[4]);
			p[5] = hex_to_int(str[5]);

			if (p[0] < 0 || p[1] < 0 || p[2] < 0 || p[3] < 0 || p[4] < 0 || p[5] < 0) {
				return false;
			}

			parts[0] = p[0] * 16 + p[1];
			parts[1] = p[2] * 16 + p[3];
			parts[2] = p[4] * 16 + p[5];

			break;
		default:
			return false;
	}

	*color = (struct wob_color) {
		.r = (float) parts[0] / UINT8_MAX,
		.g = (float) parts[1] / UINT8_MAX,
		.b = (float) parts[2] / UINT8_MAX,
		.a = (float) parts[3] / UINT8_MAX,
	};

	return true;
}

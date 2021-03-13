#define WOB_FILE "parse.c"

#include <errno.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "parse.h"
#include <limits.h>

#include "wlr-layer-shell-unstable-v1-client-protocol.h"
#include "xdg-output-unstable-v1-client-protocol.h"

bool
wob_parse_color(const char *restrict str, char **restrict str_end, struct wob_color *color)
{
	if (str[0] != '#') {
		return false;
	}
	str += 1;

	uint8_t parts[4];
	for (size_t i = 0; i < (sizeof(parts) / sizeof(uint8_t)); ++i) {
		char *strtoul_end;
		char buffer[3] = {0};

		strncpy(buffer, &str[i * 2], 2);
		parts[i] = strtoul(buffer, &strtoul_end, 16);
		if (strtoul_end != buffer + 2) {
			return false;
		}
	}

	*color = (struct wob_color){
		.alpha = parts[0] / ((float) UINT8_MAX),
		.red = parts[1] / ((float) UINT8_MAX),
		.green = parts[2] / ((float) UINT8_MAX),
		.blue = parts[3] / ((float) UINT8_MAX),
	};

	if (str_end) {
		*str_end = ((char *) str) + sizeof("FFFFFFFF") - 1;
	}

	return true;
}

bool
wob_parse_input(const char *input_buffer, unsigned long *percentage, struct wob_color *background_color, struct wob_color *border_color, struct wob_color *bar_color)
{
	char *input_ptr, *newline_position, *str_end;

	newline_position = strchr(input_buffer, '\n');
	if (newline_position == NULL) {
		return false;
	}

	if (newline_position == input_buffer) {
		return false;
	}

	*percentage = strtoul(input_buffer, &input_ptr, 10);
	if (input_ptr == newline_position) {
		return true;
	}

	struct wob_color *colors_to_parse[3] = {
		background_color,
		border_color,
		bar_color,
	};

	for (size_t i = 0; i < sizeof(colors_to_parse) / sizeof(struct wob_color *); ++i) {
		if (input_ptr[0] != ' ') {
			return false;
		}
		input_ptr += 1;

		if (!wob_parse_color(input_ptr, &str_end, colors_to_parse[i])) {
			return false;
		}

		input_ptr = str_end;
	}

	return input_ptr == newline_position;
}

bool
wob_parse_anchor(char *value, unsigned long *anchor)
{
	if (strcmp(value, "left") == 0) {
		*anchor = ZWLR_LAYER_SURFACE_V1_ANCHOR_LEFT;
	}
	else if (strcmp(value, "right") == 0) {
		*anchor = ZWLR_LAYER_SURFACE_V1_ANCHOR_RIGHT;
	}
	else if (strcmp(value, "top") == 0) {
		*anchor = ZWLR_LAYER_SURFACE_V1_ANCHOR_TOP;
	}
	else if (strcmp(value, "bottom") == 0) {
		*anchor = ZWLR_LAYER_SURFACE_V1_ANCHOR_BOTTOM;
	}
	else if (strcmp(value, "center") != 0) { // FIXME: remove last strcmp and just return false?
		return false;
	}
	else {
		return false;
	}
	return true;
}

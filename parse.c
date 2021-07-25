#define WOB_FILE "parse.c"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "parse.h"

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
		.r = (float) parts[0] / UINT8_MAX,
		.g = (float) parts[1] / UINT8_MAX,
		.b = (float) parts[2] / UINT8_MAX,
		.a = (float) parts[3] / UINT8_MAX,
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

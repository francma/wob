#define WOB_FILE "parse.c"

#include <stdlib.h>
#include <string.h>

#include "parse.h"

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

		if (!wob_color_from_string(input_ptr, &str_end, colors_to_parse[i])) {
			return false;
		}

		input_ptr = str_end;
	}

	return input_ptr == newline_position;
}

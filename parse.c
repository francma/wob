#define WOB_FILE "parse.c"

#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include "parse.h"

bool
wob_parse_color(const char *restrict str, char **restrict str_end, uint32_t *color)
{
	char *strtoul_end;

	if (str[0] != '#') {
		return false;
	}
	str += 1;

	unsigned long ul = strtoul(str, &strtoul_end, 16);
	if (ul > 0xFFFFFFFF || errno == ERANGE || (str + sizeof("FFFFFFFF") - 1) != strtoul_end) {
		return false;
	}

	*color = ul;
	if (str_end) {
		*str_end = strtoul_end;
	}

	return true;
}

bool
wob_parse_input(const char *input_buffer, unsigned long *percentage, uint32_t *background_color, uint32_t *border_color, uint32_t *bar_color)
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

	uint32_t *colors_to_parse[3] = {
		background_color,
		border_color,
		bar_color,
	};

	for (size_t i = 0; i < sizeof(colors_to_parse) / sizeof(uint32_t *); ++i) {
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
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

#include "parse.h"

int
main(int argc, char **argv)
{
	unsigned long percentage;
	struct wob_color background = {0};
	struct wob_color border = {0};
	struct wob_color bar = {0};
	char *input;
	bool result;

	printf("running 1\n");
	input = "25 #000000FF #FFFFFFFF #FFFFFFFF\n";
	result = wob_parse_input(input, &percentage, &background, &border, &bar);
	if (!result || percentage != 25 || wob_color_to_argb(background) != 0xFF000000 || wob_color_to_argb(border) != 0xFFFFFFFF || wob_color_to_argb(bar) != 0xFFFFFFFF) {
		return EXIT_FAILURE;
	}

	printf("running 2\n");
	input = "25 #000000FF\n";
	result = wob_parse_input(input, &percentage, &background, &border, &bar);
	if (result) {
		return EXIT_FAILURE;
	}

	printf("running 3\n");
	input = "25\n";
	result = wob_parse_input(input, &percentage, &background, &border, &bar);
	if (!result || percentage != 25) {
		return EXIT_FAILURE;
	}

	printf("running 4\n");
	input = "25 #000000FF #FFFFFFFF #FFFFFFFF \n";
	result = wob_parse_input(input, &percentage, &background, &border, &bar);
	if (result) {
		return EXIT_FAILURE;
	}

	printf("running 5\n");
	input = "25 #000000FF #16a085FF #FF0000FF\n";
	result = wob_parse_input(input, &percentage, &background, &border, &bar);
	if (!result || percentage != 25 || wob_color_to_argb(background) != 0xFF000000 || wob_color_to_argb(border) != 0xFF16a085 || wob_color_to_argb(bar) != 0xFFFF0000) {
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}

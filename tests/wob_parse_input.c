#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

#include "parse.h"

int
main(int argc, char **argv)
{
	unsigned long percentage;
	uint32_t background = 0;
	uint32_t border = 0;
	uint32_t bar = 0;
	char *input;
	bool result;

	printf("running 1\n");
	input = "25 #FF000000 #FFFFFFFF #FFFFFFFF\n";
	result = wob_parse_input(input, &percentage, &background, &border, &bar);
	if (!result || percentage != 25 || background != 0xFF000000 || border != 0xFFFFFFFF || bar != 0xFFFFFFFF) {
		return EXIT_FAILURE;
	}

	printf("running 2\n");
	input = "25 #FF000000\n";
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
	input = "25 #FF000000 #FFFFFFFF #FFFFFFFF \n";
	result = wob_parse_input(input, &percentage, &background, &border, &bar);
	if (result) {
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}

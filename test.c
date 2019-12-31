#define WOB_TEST
#include "wob.c"

int
main(int argc, char **argv)
{
	unsigned long percentage;
	argb_color background_color;
	argb_color border_color;
	argb_color bar_color;
	char *input;
	bool result;

	printf("running 1\n");
	input = "25 #FF000000 #FFFFFFFF #FFFFFFFF\n";
	result = wob_parse_input(input, &percentage, &background_color, &border_color, &bar_color);
	if (!result || percentage != 25 || background_color != 0xFF000000 || border_color != 0xFFFFFFFF || bar_color != 0xFFFFFFFF) {
		return EXIT_FAILURE;
	}

	printf("running 2\n");
	input = "25 #FF000000\n";
	result = wob_parse_input(input, &percentage, &background_color, &border_color, &bar_color);
	if (result) {
		return EXIT_FAILURE;
	}

	printf("running 3\n");
	input = "25\n";
	result = wob_parse_input(input, &percentage, &background_color, &border_color, &bar_color);
	if (!result || percentage != 25) {
		return EXIT_FAILURE;
	}

	printf("running 4\n");
	input = "25 #FF000000 #FFFFFFFF #FFFFFFFF \n";
	result = wob_parse_input(input, &percentage, &background_color, &border_color, &bar_color);
	if (result) {
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}
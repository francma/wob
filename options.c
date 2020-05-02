#define MIN_PERCENTAGE_BAR_WIDTH 1
#define MIN_PERCENTAGE_BAR_HEIGHT 1

#define STR(x) #x

#define _POSIX_C_SOURCE 200809L
#include <errno.h>
#include <getopt.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "options.h"
#include "parse.h"
#include "wlr-layer-shell-unstable-v1-client-protocol.h"

bool
wob_getopt(const int argc, char **argv, struct wob_options *options)
{
	const char *usage =
		"Usage: wob [options]\n"
		"\n"
		"  -h, --help                 Show help message and quit.\n"
		"  -v, --version              Show the version number and quit.\n"
		"  -t, --timeout <ms>         Hide wob after <ms> milliseconds, defaults to " STR(WOB_DEFAULT_TIMEOUT) ".\n"
		"  -m, --max <%>              Define the maximum percentage, defaults to " STR(WOB_DEFAULT_MAXIMUM) ". \n"
		"  -W, --width <px>           Define bar width in pixels, defaults to " STR(WOB_DEFAULT_WIDTH) ". \n"
		"  -H, --height <px>          Define bar height in pixels, defaults to " STR(WOB_DEFAULT_HEIGHT) ". \n"
		"  -o, --offset <px>          Define border offset in pixels, defaults to " STR(WOB_DEFAULT_BORDER_OFFSET) ". \n"
		"  -b, --border <px>          Define border size in pixels, defaults to " STR(WOB_DEFAULT_BORDER_SIZE) ". \n"
		"  -p, --padding <px>         Define bar padding in pixels, defaults to " STR(WOB_DEFAULT_BAR_PADDING) ". \n"
		"  -a, --anchor <s>           Define anchor point; one of 'top', 'left', 'right', 'bottom', 'center' (default). \n"
		"                             May be specified multiple times. \n"
		"  -M, --margin <px>          Define anchor margin in pixels, defaults to " STR(WOB_DEFAULT_MARGIN) ". \n"
		"  -O, --output <name>        Define output to show bar on or '*' for all. If ommited, focused output is chosen.\n"
		"                             May be specified multiple times.\n"
		"  --border-color <#argb>     Define border color\n"
		"  --background-color <#argb> Define background color\n"
		"  --bar-color <#argb>        Define bar color\n"
		"\n";

	*options = (struct wob_options){
		.bar_width = WOB_DEFAULT_WIDTH,
		.bar_height = WOB_DEFAULT_HEIGHT,
		.border_offset = WOB_DEFAULT_BORDER_OFFSET,
		.border_size = WOB_DEFAULT_BORDER_SIZE,
		.bar_padding = WOB_DEFAULT_BAR_PADDING,
		.bar_anchor = WOB_DEFAULT_ANCHOR,
		.bar_margin = WOB_DEFAULT_MARGIN,
		.background_color = WOB_DEFAULT_BACKGROUND_COLOR,
		.border_color = WOB_DEFAULT_BORDER_COLOR,
		.bar_color = WOB_DEFAULT_BAR_COLOR,
		.maximum = WOB_DEFAULT_MAXIMUM,
		.timeout_msec = WOB_DEFAULT_TIMEOUT,
		.pledge = WOB_DEFAULT_PLEDGE,
	};

	char *disable_pledge_env = getenv("WOB_DISABLE_PLEDGE");
	if (disable_pledge_env != NULL && strcmp(disable_pledge_env, "0") != 0) {
		options->pledge = false;
	}

	wl_list_init(&(options->outputs));

	char *strtoul_end;

	static struct option long_options[] = {
		{"help", no_argument, NULL, 'h'},
		{"version", no_argument, NULL, 'v'},
		{"timeout", required_argument, NULL, 't'},
		{"max", required_argument, NULL, 'm'},
		{"width", required_argument, NULL, 'W'},
		{"height", required_argument, NULL, 'H'},
		{"offset", required_argument, NULL, 'o'},
		{"border", required_argument, NULL, 'b'},
		{"padding", required_argument, NULL, 'p'},
		{"anchor", required_argument, NULL, 'a'},
		{"margin", required_argument, NULL, 'M'},
		{"output", required_argument, NULL, 'O'},
		{"border-color", required_argument, NULL, 1},
		{"background-color", required_argument, NULL, 2},
		{"bar-color", required_argument, NULL, 3},
	};

	int option_index = 0;
	int c;
	struct wob_output_name *output;

	while ((c = getopt_long(argc, argv, "t:m:W:H:o:b:p:a:M:O:vh", long_options, &option_index)) != -1) {
		switch (c) {
			case 1:
				if (!wob_parse_color(optarg, &strtoul_end, &(options->border_color))) {
					fprintf(stderr, "Border color must be a value between #00000000 and #FFFFFFFF.\n");
					return false;
				}
				break;
			case 2:
				if (!wob_parse_color(optarg, &strtoul_end, &(options->background_color))) {
					fprintf(stderr, "Background color must be a value between #00000000 and #FFFFFFFF.\n");
					return false;
				}
				break;
			case 3:
				if (!wob_parse_color(optarg, &strtoul_end, &(options->bar_color))) {
					fprintf(stderr, "Bar color must be a value between #00000000 and #FFFFFFFF.\n");
					return false;
				}
				break;
			case 't':
				options->timeout_msec = strtoul(optarg, &strtoul_end, 10);
				if (*strtoul_end != '\0' || errno == ERANGE || options->timeout_msec == 0) {
					fprintf(stderr, "Timeout must be a value between 1 and %lu.\n", ULONG_MAX);
					return false;
				}
				break;
			case 'm':
				options->maximum = strtoul(optarg, &strtoul_end, 10);
				if (*strtoul_end != '\0' || errno == ERANGE || options->maximum == 0) {
					fprintf(stderr, "Maximum must be a value between 1 and %lu.\n", ULONG_MAX);
					return false;
				}
				break;
			case 'W':
				options->bar_width = strtoul(optarg, &strtoul_end, 10);
				if (*strtoul_end != '\0' || errno == ERANGE) {
					fprintf(stderr, "Width must be a positive value.");
					return false;
				}
				break;
			case 'H':
				options->bar_height = strtoul(optarg, &strtoul_end, 10);
				if (*strtoul_end != '\0' || errno == ERANGE) {
					fprintf(stderr, "Height must be a positive value.");
					return false;
				}
				break;
			case 'o':
				options->border_offset = strtoul(optarg, &strtoul_end, 10);
				if (*strtoul_end != '\0' || errno == ERANGE) {
					fprintf(stderr, "Border offset must be a positive value.");
					return false;
				}
				break;
			case 'b':
				options->border_size = strtoul(optarg, &strtoul_end, 10);
				if (*strtoul_end != '\0' || errno == ERANGE) {
					fprintf(stderr, "Border size must be a positive value.");
					return false;
				}
				break;
			case 'p':
				options->bar_padding = strtoul(optarg, &strtoul_end, 10);
				if (*strtoul_end != '\0' || errno == ERANGE) {
					fprintf(stderr, "Bar padding must be a positive value.");
					return false;
				}
				break;
			case 'a':
				if (strcmp(optarg, "left") == 0) {
					options->bar_anchor |= ZWLR_LAYER_SURFACE_V1_ANCHOR_LEFT;
				}
				else if (strcmp(optarg, "right") == 0) {
					options->bar_anchor |= ZWLR_LAYER_SURFACE_V1_ANCHOR_RIGHT;
				}
				else if (strcmp(optarg, "top") == 0) {
					options->bar_anchor |= ZWLR_LAYER_SURFACE_V1_ANCHOR_TOP;
				}
				else if (strcmp(optarg, "bottom") == 0) {
					options->bar_anchor |= ZWLR_LAYER_SURFACE_V1_ANCHOR_BOTTOM;
				}
				else if (strcmp(optarg, "center") != 0) {
					fprintf(stderr, "Anchor must be one of 'top', 'bottom', 'left', 'right', 'center'.");
					return false;
				}
				break;
			case 'M':
				options->bar_margin = strtoul(optarg, &strtoul_end, 10);
				if (*strtoul_end != '\0' || errno == ERANGE) {
					fprintf(stderr, "Anchor margin must be a positive value.");
					return false;
				}
				break;
			case 'O':
				output = calloc(1, sizeof(struct wob_output_name));
				if (output == NULL) {
					fprintf(stderr, "calloc failed\n");
					return false;
				}

				output->name = strdup(optarg);
				if (output->name == NULL) {
					fprintf(stderr, "strdup failed\n");
					return false;
				}

				wl_list_insert(&(options->outputs), &(output->link));
				break;
			case 'v':
				fprintf(stdout, "wob version: " WOB_VERSION "\n");
				return true;
			case 'h':
				fprintf(stdout, "%s", usage);
				return true;
			default:
				fprintf(stderr, "%s", usage);
				return false;
		}
	}

	if (options->bar_width < MIN_PERCENTAGE_BAR_WIDTH + 2 * (options->border_offset + options->border_size + options->bar_padding)) {
		fprintf(stderr, "Invalid geometry: width is too small for given parameters\n");
		return false;
	}

	if (options->bar_height < MIN_PERCENTAGE_BAR_HEIGHT + 2 * (options->border_offset + options->border_size + options->bar_padding)) {
		fprintf(stderr, "Invalid geometry: height is too small for given parameters\n");
		return false;
	}

	return true;
}

void
wob_options_destroy(struct wob_options *options)
{
	struct wob_output_name *output_name, *output_name_tmp;
	wl_list_for_each_safe (output_name, output_name_tmp, &options->outputs, link) {
		free(output_name->name);
		free(output_name);
	}
}
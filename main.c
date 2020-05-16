#define WOB_FILE "main.c"

#define WOB_DEFAULT_WIDTH 400
#define WOB_DEFAULT_HEIGHT 50
#define WOB_DEFAULT_BORDER_OFFSET 4
#define WOB_DEFAULT_BORDER_SIZE 4
#define WOB_DEFAULT_BAR_PADDING 4
#define WOB_DEFAULT_ANCHOR 0
#define WOB_DEFAULT_MARGIN 0
#define WOB_DEFAULT_MAXIMUM 100
#define WOB_DEFAULT_TIMEOUT 1000
#define WOB_DEFAULT_BAR_COLOR 0xFFFFFFFF
#define WOB_DEFAULT_BACKGROUND_COLOR 0xFF000000
#define WOB_DEFAULT_BORDER_COLOR 0xFFFFFFFF

#define MIN_PERCENTAGE_BAR_WIDTH 1
#define MIN_PERCENTAGE_BAR_HEIGHT 1

#define STR(x) #x

// sizeof already includes NULL byte
#define INPUT_BUFFER_LENGTH (3 * sizeof(unsigned long) + sizeof(" #FF000000 #FFFFFFFF #FFFFFFFF\n"))

#define MIN(a, b) (((a) < (b)) ? (a) : (b))

#define _POSIX_C_SOURCE 200809L
#include <errno.h>
#include <getopt.h>
#include <limits.h>
#include <poll.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "buffer.h"
#include "log.h"
#include "parse.h"
#include "pledge.h"
#include "wlr-layer-shell-unstable-v1-client-protocol.h"
#include "xdg-output-unstable-v1-client-protocol.h"

struct wob_geom {
	unsigned long width;
	unsigned long height;
	unsigned long border_offset;
	unsigned long border_size;
	unsigned long bar_padding;
	unsigned long stride;
	unsigned long size;
	unsigned long anchor;
	unsigned long margin;
};

struct wob_colors {
	uint32_t bar;
	uint32_t background;
	uint32_t border;
};

struct wob_output_config {
	char *name;
	struct wl_list link;
};

struct wob_surface {
	struct zwlr_layer_surface_v1 *wlr_layer_surface;
	struct wl_surface *wl_surface;
};

struct wob_output {
	char *name;
	struct wl_list link;
	struct wl_output *wl_output;
	struct wob *app;
	struct wob_surface *wob_surface;
	struct zxdg_output_v1 *xdg_output;
	uint32_t wl_name;
};

struct wob {
	int shmid;
	struct wl_buffer *wl_buffer;
	struct wl_compositor *wl_compositor;
	struct wl_display *wl_display;
	struct wl_list wob_outputs;
	struct wl_list output_configs;
	struct wl_registry *wl_registry;
	struct wl_shm *wl_shm;
	struct wob_geom *wob_geom;
	struct zwlr_layer_shell_v1 *wlr_layer_shell;
	struct zxdg_output_manager_v1 *xdg_output_manager;
	struct wob_surface *fallback_wob_surface;
};

void
noop()
{
	/* intentionally left blank */
}

void
layer_surface_configure(void *data, struct zwlr_layer_surface_v1 *surface, uint32_t serial, uint32_t w, uint32_t h)
{
	zwlr_layer_surface_v1_ack_configure(surface, serial);
}

void
xdg_output_handle_name(void *data, struct zxdg_output_v1 *xdg_output, const char *name)
{
	struct wob_output *output = (struct wob_output *) data;
	output->name = strdup(name);
	if (output->name == NULL) {
		wob_log_error("strdup failed\n");
		exit(EXIT_FAILURE);
	}
}

struct wob_surface *
wob_surface_create(struct wob *app, struct wl_output *wl_output)
{
	const static struct zwlr_layer_surface_v1_listener zwlr_layer_surface_listener = {
		.configure = layer_surface_configure,
		.closed = noop,
	};

	struct wob_surface *wob_surface = calloc(1, sizeof(struct wob_surface));
	if (wob_surface == NULL) {
		wob_log_error("calloc failed");
		exit(EXIT_FAILURE);
	}

	wob_surface->wl_surface = wl_compositor_create_surface(app->wl_compositor);
	if (wob_surface->wl_surface == NULL) {
		wob_log_error("wl_compositor_create_surface failed");
		exit(EXIT_FAILURE);
	}
	wob_surface->wlr_layer_surface = zwlr_layer_shell_v1_get_layer_surface(app->wlr_layer_shell, wob_surface->wl_surface, wl_output, ZWLR_LAYER_SHELL_V1_LAYER_OVERLAY, "wob");
	if (wob_surface->wlr_layer_surface == NULL) {
		wob_log_error("wlr_layer_shell_v1_get_layer_surface failed");
		exit(EXIT_FAILURE);
	}
	zwlr_layer_surface_v1_set_size(wob_surface->wlr_layer_surface, app->wob_geom->width, app->wob_geom->height);
	zwlr_layer_surface_v1_set_anchor(wob_surface->wlr_layer_surface, app->wob_geom->anchor);
	zwlr_layer_surface_v1_set_margin(wob_surface->wlr_layer_surface, app->wob_geom->margin, app->wob_geom->margin, app->wob_geom->margin, app->wob_geom->margin);
	zwlr_layer_surface_v1_add_listener(wob_surface->wlr_layer_surface, &zwlr_layer_surface_listener, app);
	wl_surface_commit(wob_surface->wl_surface);

	return wob_surface;
}

void
wob_surface_destroy(struct wob_surface *wob_surface)
{
	if (wob_surface == NULL) {
		return;
	}

	zwlr_layer_surface_v1_destroy(wob_surface->wlr_layer_surface);
	wl_surface_destroy(wob_surface->wl_surface);

	wob_surface->wl_surface = NULL;
	wob_surface->wlr_layer_surface = NULL;
}

void
wob_output_destroy(struct wob_output *output)
{
	wob_surface_destroy(output->wob_surface);
	zxdg_output_v1_destroy(output->xdg_output);
	wl_output_destroy(output->wl_output);

	free(output->name);
	free(output->wob_surface);

	output->wob_surface = NULL;
	output->wl_output = NULL;
	output->xdg_output = NULL;
	output->name = NULL;
}

void
xdg_output_handle_done(void *data, struct zxdg_output_v1 *xdg_output)
{
	struct wob_output *output = (struct wob_output *) data;
	struct wob *app = output->app;

	struct wob_output_config *output_config, *tmp;
	wl_list_for_each_safe (output_config, tmp, &app->output_configs, link) {
		if (strcmp(output->name, output_config->name) == 0 || strcmp("*", output_config->name) == 0) {
			wl_list_insert(&output->app->wob_outputs, &output->link);
			return;
		}
	}

	wob_output_destroy(output);
	free(output);
}

void
handle_global(void *data, struct wl_registry *registry, uint32_t name, const char *interface, uint32_t version)
{
	const static struct zxdg_output_v1_listener xdg_output_listener = {
		.logical_position = noop,
		.logical_size = noop,
		.name = xdg_output_handle_name,
		.description = noop,
		.done = xdg_output_handle_done,
	};

	struct wob *app = (struct wob *) data;

	if (strcmp(interface, wl_shm_interface.name) == 0) {
		app->wl_shm = wl_registry_bind(registry, name, &wl_shm_interface, 1);
	}
	else if (strcmp(interface, wl_compositor_interface.name) == 0) {
		app->wl_compositor = wl_registry_bind(registry, name, &wl_compositor_interface, 1);
	}
	else if (strcmp(interface, "wl_output") == 0) {
		if (!wl_list_empty(&(app->output_configs))) {
			struct wob_output *output = calloc(1, sizeof(struct wob_output));
			output->wl_output = wl_registry_bind(registry, name, &wl_output_interface, 1);
			output->app = app;
			output->wl_name = name;

			output->xdg_output = zxdg_output_manager_v1_get_xdg_output(app->xdg_output_manager, output->wl_output);
			zxdg_output_v1_add_listener(output->xdg_output, &xdg_output_listener, output);

			if (wl_display_roundtrip(app->wl_display) == -1) {
				wob_log_error("wl_display_roundtrip failed");
				exit(EXIT_FAILURE);
			}
		}
	}
	else if (strcmp(interface, zwlr_layer_shell_v1_interface.name) == 0) {
		app->wlr_layer_shell = wl_registry_bind(registry, name, &zwlr_layer_shell_v1_interface, 1);
	}
	else if (strcmp(interface, zxdg_output_manager_v1_interface.name) == 0) {
		app->xdg_output_manager = wl_registry_bind(registry, name, &zxdg_output_manager_v1_interface, 2);
	}
}

void
handle_global_remove(void *data, struct wl_registry *registry, uint32_t name)
{
	struct wob *app = (struct wob *) data;
	struct wob_output *output, *tmp;
	wl_list_for_each_safe (output, tmp, &(app->wob_outputs), link) {
		if (output->wl_name == name) {
			wob_output_destroy(output);
			break;
		}
	}
}

void
wob_flush(struct wob *app)
{

	if (wl_list_empty(&(app->wob_outputs))) {
		wl_surface_attach(app->fallback_wob_surface->wl_surface, app->wl_buffer, 0, 0);
		wl_surface_damage(app->fallback_wob_surface->wl_surface, 0, 0, app->wob_geom->width, app->wob_geom->height);
		wl_surface_commit(app->fallback_wob_surface->wl_surface);
	}
	else {
		struct wob_output *output, *tmp;
		wl_list_for_each_safe (output, tmp, &(app->wob_outputs), link) {
			wl_surface_attach(output->wob_surface->wl_surface, app->wl_buffer, 0, 0);
			wl_surface_damage(output->wob_surface->wl_surface, 0, 0, app->wob_geom->width, app->wob_geom->height);
			wl_surface_commit(output->wob_surface->wl_surface);
		}
	}

	if (wl_display_dispatch(app->wl_display) == -1) {
		wob_log_error("wl_display_dispatch failed");
		exit(EXIT_FAILURE);
	}
}

void
wob_hide(struct wob *app)
{
	if (wl_list_empty(&(app->wob_outputs))) {
		wob_surface_destroy(app->fallback_wob_surface);
		free(app->fallback_wob_surface);
		app->fallback_wob_surface = NULL;
	}
	else {
		struct wob_output *output, *tmp;
		wl_list_for_each_safe (output, tmp, &app->wob_outputs, link) {
			wob_surface_destroy(output->wob_surface);
			free(output->wob_surface);
			output->wob_surface = NULL;
		}
	}

	if (wl_display_roundtrip(app->wl_display) == -1) {
		wob_log_error("wl_display_roundtrip failed");
		exit(EXIT_FAILURE);
	}
}

void
wob_show(struct wob *app)
{
	if (wl_list_empty(&(app->wob_outputs))) {
		app->fallback_wob_surface = wob_surface_create(app, NULL);
	}
	else {
		struct wob_output *output, *tmp;
		wl_list_for_each_safe (output, tmp, &app->wob_outputs, link) {
			output->wob_surface = wob_surface_create(app, output->wl_output);
		}
	}

	if (wl_display_roundtrip(app->wl_display) == -1) {
		wob_log_error("wl_display_roundtrip failed");
		exit(EXIT_FAILURE);
	}
}

void
wob_destroy(struct wob *app)
{
	struct wob_output *output, *output_tmp;
	wl_list_for_each_safe (output, output_tmp, &app->wob_outputs, link) {
		wob_output_destroy(output);
		free(output);
	}

	struct wob_output_config *config, *config_tmp;
	wl_list_for_each_safe (config, config_tmp, &app->output_configs, link) {
		free(config->name);
		free(config);
	}

	zwlr_layer_shell_v1_destroy(app->wlr_layer_shell);
	wl_registry_destroy(app->wl_registry);
	wl_buffer_destroy(app->wl_buffer);
	wl_compositor_destroy(app->wl_compositor);
	wl_shm_destroy(app->wl_shm);
	zxdg_output_manager_v1_destroy(app->xdg_output_manager);

	wl_display_disconnect(app->wl_display);
}

void
wob_connect(struct wob *app)
{
	const static struct wl_registry_listener wl_registry_listener = {
		.global = handle_global,
		.global_remove = noop,
	};

	app->wl_display = wl_display_connect(NULL);
	if (app->wl_display == NULL) {
		wob_log_error("wl_display_connect failed");
		exit(EXIT_FAILURE);
	}

	app->wl_registry = wl_display_get_registry(app->wl_display);
	if (app->wl_registry == NULL) {
		wob_log_error("wl_display_get_registry failed");
		exit(EXIT_FAILURE);
	}

	wl_registry_add_listener(app->wl_registry, &wl_registry_listener, app);

	wl_list_init(&app->wob_outputs);
	if (wl_display_roundtrip(app->wl_display) == -1) {
		wob_log_error("wl_display_roundtrip failed");
		exit(EXIT_FAILURE);
	}

	struct wl_shm_pool *pool = wl_shm_create_pool(app->wl_shm, app->shmid, app->wob_geom->size);
	if (pool == NULL) {
		wob_log_error("wl_shm_create_pool failed");
		exit(EXIT_FAILURE);
	}

	app->wl_buffer = wl_shm_pool_create_buffer(pool, 0, app->wob_geom->width, app->wob_geom->height, app->wob_geom->stride, WL_SHM_FORMAT_ARGB8888);
	wl_shm_pool_destroy(pool);
	if (app->wl_buffer == NULL) {
		wob_log_error("wl_shm_pool_create_buffer failed");
		exit(EXIT_FAILURE);
	}
}

void
wob_draw_background(const struct wob_geom *geom, uint32_t *argb, uint32_t color)
{
	for (size_t i = 0; i < geom->width * geom->height; ++i) {
		argb[i] = color;
	}
}

void
wob_draw_border(const struct wob_geom *geom, uint32_t *argb, uint32_t color)
{
	// create top and bottom line
	size_t i = geom->width * geom->border_offset;
	size_t k = geom->width * (geom->height - geom->border_offset - geom->border_size);
	for (size_t line = 0; line < geom->border_size; ++line) {
		i += geom->border_offset;
		k += geom->border_offset;
		for (size_t pixel = 0; pixel < geom->width - 2 * geom->border_offset; ++pixel) {
			argb[i++] = color;
			argb[k++] = color;
		}
		i += geom->border_offset;
		k += geom->border_offset;
	}

	// create left and right horizontal line
	i = geom->width * (geom->border_offset + geom->border_size);
	k = geom->width * (geom->border_offset + geom->border_size);
	for (size_t line = 0; line < geom->height - 2 * (geom->border_size + geom->border_offset); ++line) {
		i += geom->border_offset;
		k += geom->width - geom->border_offset - geom->border_size;
		for (size_t pixel = 0; pixel < geom->border_size; ++pixel) {
			argb[i++] = color;
			argb[k++] = color;
		}
		i += geom->width - geom->border_offset - geom->border_size;
		k += geom->border_offset;
	}
}

void
wob_draw_percentage(const struct wob_geom *geom, uint32_t *argb, uint32_t bar_color, uint32_t background_color, unsigned long percentage, unsigned long maximum)
{
	size_t offset_border_padding = geom->border_offset + geom->border_size + geom->bar_padding;
	size_t bar_width = geom->width - 2 * offset_border_padding;
	size_t bar_height = geom->height - 2 * offset_border_padding;
	size_t bar_colored_width = (bar_width * percentage) / maximum;

	// draw 1px horizontal line
	uint32_t *start, *end, *pixel;
	start = &argb[offset_border_padding * (geom->width + 1)];
	end = start + bar_colored_width;
	for (pixel = start; pixel < end; ++pixel) {
		*pixel = bar_color;
	}
	for (end = start + bar_width; pixel < end; ++pixel) {
		*pixel = background_color;
	}

	// copy it to make full percentage bar
	uint32_t *source = &argb[offset_border_padding * geom->width];
	uint32_t *destination = source + geom->width;
	end = &argb[geom->width * (bar_height + offset_border_padding)];
	while (destination != end) {
		memcpy(destination, source, MIN(destination - source, end - destination) * sizeof(uint32_t));
		destination += MIN(destination - source, end - destination);
	}
}

int
main(int argc, char **argv)
{
	wob_log_use_colors(isatty(STDERR_FILENO));
	wob_log_level_warn();

	const char *usage =
		"Usage: wob [options]\n"
		"\n"
		"  -h, --help                 Show help message and quit.\n"
		"  --version                  Show the version number and quit.\n"
		"  -v                         Increase verbosity of messages, defaults to errors and warnings only\n"
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

	struct wob app = {0};
	wl_list_init(&(app.output_configs));

	unsigned long maximum = WOB_DEFAULT_MAXIMUM;
	unsigned long timeout_msec = WOB_DEFAULT_TIMEOUT;
	struct wob_geom geom = {
		.width = WOB_DEFAULT_WIDTH,
		.height = WOB_DEFAULT_HEIGHT,
		.border_offset = WOB_DEFAULT_BORDER_OFFSET,
		.border_size = WOB_DEFAULT_BORDER_SIZE,
		.bar_padding = WOB_DEFAULT_BAR_PADDING,
		.anchor = WOB_DEFAULT_ANCHOR,
		.margin = WOB_DEFAULT_MARGIN,
	};
	struct wob_colors colors = {
		.background = WOB_DEFAULT_BACKGROUND_COLOR,
		.bar = WOB_DEFAULT_BAR_COLOR,
		.border = WOB_DEFAULT_BORDER_COLOR,
	};
	bool pledge = true;

	char *disable_pledge_env = getenv("WOB_DISABLE_PLEDGE");
	if (disable_pledge_env != NULL && strcmp(disable_pledge_env, "0") != 0) {
		pledge = false;
	}

	struct wob_output_config *output_config;
	int option_index = 0;
	int c;
	char *strtoul_end;
	static struct option long_options[] = {
		{"help", no_argument, NULL, 'h'},
		{"version", no_argument, NULL, 4},
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
		{"verbose", no_argument, NULL, 'v'},
	};
	while ((c = getopt_long(argc, argv, "t:m:W:H:o:b:p:a:M:O:vh", long_options, &option_index)) != -1) {
		switch (c) {
			case 1:
				if (!wob_parse_color(optarg, &strtoul_end, &(colors.border))) {
					wob_log_error("Border color must be a value between #00000000 and #FFFFFFFF.");
					return EXIT_FAILURE;
				}
				break;
			case 2:
				if (!wob_parse_color(optarg, &strtoul_end, &(colors.background))) {
					wob_log_error("Background color must be a value between #00000000 and #FFFFFFFF.");
					return EXIT_FAILURE;
				}
				break;
			case 3:
				if (!wob_parse_color(optarg, &strtoul_end, &(colors.bar))) {
					wob_log_error("Bar color must be a value between #00000000 and #FFFFFFFF.");
					return EXIT_FAILURE;
				}
				break;
			case 't':
				timeout_msec = strtoul(optarg, &strtoul_end, 10);
				if (*strtoul_end != '\0' || errno == ERANGE || timeout_msec == 0) {
					wob_log_error("Timeout must be a value between 1 and %lu.", ULONG_MAX);
					return EXIT_FAILURE;
				}
				break;
			case 'm':
				maximum = strtoul(optarg, &strtoul_end, 10);
				if (*strtoul_end != '\0' || errno == ERANGE || maximum == 0) {
					wob_log_error("Maximum must be a value between 1 and %lu.", ULONG_MAX);
					return EXIT_FAILURE;
				}
				break;
			case 'W':
				geom.width = strtoul(optarg, &strtoul_end, 10);
				if (*strtoul_end != '\0' || errno == ERANGE) {
					wob_log_error("Width must be a positive value.");
					return EXIT_FAILURE;
				}
				break;
			case 'H':
				geom.height = strtoul(optarg, &strtoul_end, 10);
				if (*strtoul_end != '\0' || errno == ERANGE) {
					wob_log_error("Height must be a positive value.");
					return EXIT_FAILURE;
				}
				break;
			case 'o':
				geom.border_offset = strtoul(optarg, &strtoul_end, 10);
				if (*strtoul_end != '\0' || errno == ERANGE) {
					wob_log_error("Border offset must be a positive value.");
					return EXIT_FAILURE;
				}
				break;
			case 'b':
				geom.border_size = strtoul(optarg, &strtoul_end, 10);
				if (*strtoul_end != '\0' || errno == ERANGE) {
					wob_log_error("Border size must be a positive value.");
					return EXIT_FAILURE;
				}
				break;
			case 'p':
				geom.bar_padding = strtoul(optarg, &strtoul_end, 10);
				if (*strtoul_end != '\0' || errno == ERANGE) {
					wob_log_error("Bar padding must be a positive value.");
					return EXIT_FAILURE;
				}
				break;
			case 'a':
				if (strcmp(optarg, "left") == 0) {
					geom.anchor |= ZWLR_LAYER_SURFACE_V1_ANCHOR_LEFT;
				}
				else if (strcmp(optarg, "right") == 0) {
					geom.anchor |= ZWLR_LAYER_SURFACE_V1_ANCHOR_RIGHT;
				}
				else if (strcmp(optarg, "top") == 0) {
					geom.anchor |= ZWLR_LAYER_SURFACE_V1_ANCHOR_TOP;
				}
				else if (strcmp(optarg, "bottom") == 0) {
					geom.anchor |= ZWLR_LAYER_SURFACE_V1_ANCHOR_BOTTOM;
				}
				else if (strcmp(optarg, "center") != 0) {
					wob_log_error("Anchor must be one of 'top', 'bottom', 'left', 'right', 'center'.");
					return EXIT_FAILURE;
				}
				break;
			case 'M':
				geom.margin = strtoul(optarg, &strtoul_end, 10);
				if (*strtoul_end != '\0' || errno == ERANGE) {
					wob_log_error("Anchor margin must be a positive value.");
					return EXIT_FAILURE;
				}
				break;
			case 'O':
				output_config = calloc(1, sizeof(struct wob_output_config));
				if (output_config == NULL) {
					wob_log_error("calloc failed");
					return EXIT_FAILURE;
				}

				output_config->name = strdup(optarg);
				if (output_config->name == NULL) {
					wob_log_error("strdup failed");
					return EXIT_FAILURE;
				}

				wl_list_insert(&(app.output_configs), &(output_config->link));
				break;
			case 4:
				printf("wob version: " WOB_VERSION "\n");
				return EXIT_SUCCESS;
			case 'h':
				printf("%s", usage);
				return EXIT_SUCCESS;
			case 'v':
				wob_log_inc_verbosity();
				break;
			default:
				fprintf(stderr, "%s", usage);
				return EXIT_FAILURE;
		}
	}

	if (geom.width < MIN_PERCENTAGE_BAR_WIDTH + 2 * (geom.border_offset + geom.border_size + geom.bar_padding)) {
		wob_log_error("Invalid geometry: width is too small for given parameters");
		return EXIT_FAILURE;
	}

	if (geom.height < MIN_PERCENTAGE_BAR_HEIGHT + 2 * (geom.border_offset + geom.border_size + geom.bar_padding)) {
		wob_log_error("Invalid geometry: height is too small for given parameters");
		return EXIT_FAILURE;
	}

	geom.stride = geom.width * 4;
	geom.size = geom.stride * geom.height;
	app.wob_geom = &geom;

	int shmid = wob_shm_create();
	if (shmid < 0) {
		return EXIT_FAILURE;
	}
	app.shmid = shmid;

	uint32_t *argb = wob_shm_alloc(shmid, app.wob_geom->size);
	if (argb == NULL) {
		return EXIT_FAILURE;
	}

	wob_connect(&app);

	if (pledge) {
		if (!wob_pledge()) {
			return EXIT_FAILURE;
		}
	}

	struct wob_colors old_colors = {0};

	// Draw these at least once
	wob_draw_background(app.wob_geom, argb, colors.background);
	wob_draw_border(app.wob_geom, argb, colors.border);

	struct pollfd fds[2] = {
		{
			.fd = wl_display_get_fd(app.wl_display),
			.events = POLLIN,
		},
		{
			.fd = STDIN_FILENO,
			.events = POLLIN,
		},
	};

	bool hidden = true;
	for (;;) {
		unsigned long percentage = 0;
		char input_buffer[INPUT_BUFFER_LENGTH] = {0};
		char *fgets_rv;

		switch (poll(fds, 2, hidden ? -1 : timeout_msec)) {
			case -1:
				wob_log_error("poll() failed: %s", strerror(errno));
				return EXIT_FAILURE;
			case 0:
				if (!hidden) {
					wob_log_debug("Hide");
					wob_hide(&app);
				}

				hidden = true;
				break;
			default:
				if (fds[0].revents & POLLIN) {
					if (wl_display_dispatch(app.wl_display) == -1) {
						return EXIT_FAILURE;
					}
				}

				if (!(fds[1].revents & POLLIN)) {
					break;
				}

				fgets_rv = fgets(input_buffer, INPUT_BUFFER_LENGTH, stdin);
				if (feof(stdin)) {
					if (!hidden) {
						wob_hide(&app);
					}
					wob_log_info("Received EOF");
					wob_destroy(&app);
					return EXIT_SUCCESS;
				}

				old_colors = colors;
				if (fgets_rv == NULL || !wob_parse_input(input_buffer, &percentage, &colors.background, &colors.border, &colors.bar) || percentage > maximum) {
					wob_log_error("Received invalid input");
					if (!hidden) {
						wob_hide(&app);
					}
					wob_destroy(&app);
					return EXIT_FAILURE;
				}

				wob_log_info("Received input { value = %ld, bg = %#x, border = %#x, bar = %#x }", percentage, colors.background, colors.border, colors.bar);

				if (hidden) {
					wob_log_debug("Show");
					wob_show(&app);
				}

				if (old_colors.background != colors.background || old_colors.border != colors.border) {
					wob_draw_background(app.wob_geom, argb, colors.background);
					wob_draw_border(app.wob_geom, argb, colors.border);
				}

				wob_draw_percentage(app.wob_geom, argb, colors.bar, colors.background, percentage, maximum);

				wob_flush(&app);
				hidden = false;

				break;
		}
	}
}

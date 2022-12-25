#define WOB_FILE "main.c"

#define MIN_PERCENTAGE_BAR_WIDTH 1
#define MIN_PERCENTAGE_BAR_HEIGHT 1

#define INPUT_BUFFER_LENGTH 255

#define MIN(a, b) (((a) < (b)) ? (a) : (b))

#define STDIN_BUFFER_LENGTH INPUT_BUFFER_LENGTH

#define _POSIX_C_SOURCE 200809L
#include <errno.h>
#include <getopt.h>
#include <pixman.h>
#include <poll.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "buffer.h"
#include "color.h"
#include "config.h"
#include "log.h"
#include "pledge.h"
#include "wlr-layer-shell-unstable-v1-client-protocol.h"
#include "xdg-output-unstable-v1-client-protocol.h"

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
	struct wl_registry *wl_registry;
	struct wl_shm *wl_shm;
	struct zwlr_layer_shell_v1 *wlr_layer_shell;
	struct zxdg_output_manager_v1 *xdg_output_manager;
	struct wob_config wob_config;
};

unsigned long
wob_anchor_to_wlr_layer_surface_anchor(enum wob_anchor wob_anchor)
{
	unsigned long wlr_layer_surface_anchor = 0;

	if (WOB_ANCHOR_TOP & wob_anchor) {
		wlr_layer_surface_anchor |= ZWLR_LAYER_SURFACE_V1_ANCHOR_TOP;
	}

	if (WOB_ANCHOR_RIGHT & wob_anchor) {
		wlr_layer_surface_anchor |= ZWLR_LAYER_SURFACE_V1_ANCHOR_RIGHT;
	}

	if (WOB_ANCHOR_BOTTOM & wob_anchor) {
		wlr_layer_surface_anchor |= ZWLR_LAYER_SURFACE_V1_ANCHOR_BOTTOM;
	}

	if (WOB_ANCHOR_LEFT & wob_anchor) {
		wlr_layer_surface_anchor |= ZWLR_LAYER_SURFACE_V1_ANCHOR_LEFT;
	}

	return wlr_layer_surface_anchor;
}

pixman_color_t
wob_color_to_pixman(struct wob_color color)
{
	return (pixman_color_t){
		.red = (uint16_t) (color.r * UINT16_MAX),
		.green = (uint16_t) (color.g * UINT16_MAX),
		.blue = (uint16_t) (color.b * UINT16_MAX),
		.alpha = (uint16_t) (color.a * UINT16_MAX),
	};
}

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

struct wob_surface *
wob_surface_create(struct wob *app, struct wl_output *wl_output)
{
	struct wob_config config = app->wob_config;

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

	struct wob_dimensions dimensions = config.dimensions;
	struct wob_margin margin = config.margin;
	enum wob_anchor anchor = config.anchor;

	zwlr_layer_surface_v1_set_size(wob_surface->wlr_layer_surface, dimensions.width, dimensions.height);
	zwlr_layer_surface_v1_set_anchor(wob_surface->wlr_layer_surface, wob_anchor_to_wlr_layer_surface_anchor(anchor));
	zwlr_layer_surface_v1_set_margin(wob_surface->wlr_layer_surface, margin.top, margin.right, margin.bottom, margin.left);
	zwlr_layer_surface_v1_add_listener(wob_surface->wlr_layer_surface, &zwlr_layer_surface_listener, app);
	wl_surface_commit(wob_surface->wl_surface);

	return wob_surface;
}

void
wob_surface_destroy(struct wob_surface *wob_surface)
{
	zwlr_layer_surface_v1_destroy(wob_surface->wlr_layer_surface);
	wl_surface_destroy(wob_surface->wl_surface);

	wob_surface->wl_surface = NULL;
	wob_surface->wlr_layer_surface = NULL;
}

void
wob_output_destroy(struct wob_output *output)
{
	if (output->wob_surface != NULL) {
		wob_surface_destroy(output->wob_surface);
	}
	if (output->xdg_output != NULL) {
		zxdg_output_v1_destroy(output->xdg_output);
	}
	if (output->wl_output != NULL) {
		wl_output_destroy(output->wl_output);
	}

	free(output->name);
	free(output->wob_surface);

	output->wob_surface = NULL;
	output->wl_output = NULL;
	output->xdg_output = NULL;
	output->name = NULL;
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

void
xdg_output_handle_done(void *data, struct zxdg_output_v1 *xdg_output)
{
	struct wob_output *output = (struct wob_output *) data;
	struct wob *app = output->app;

	struct wob_output_config *output_config;

	wob_log_info("Detected output name %s", output->name);
	if (app->wob_config.output_mode == WOB_OUTPUT_MODE_ALL) {
		wl_list_insert(&output->app->wob_outputs, &output->link);
		wob_log_info("Bar will be displayed on %s, because 'output_mode = all' is selected", output->name);
		return;
	}

	wl_list_for_each (output_config, &app->wob_config.outputs, link) {
		if (strcmp(output->name, output_config->name) == 0) {
			wl_list_insert(&output->app->wob_outputs, &output->link);
			wob_log_info("Bar will be displayed on output %s, because it matches whitelist rule", output->name);
			return;
		}
	}

	wob_log_info("Bar will NOT be displayed on output %s", output->name);

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
		if (app->wob_config.output_mode != WOB_OUTPUT_MODE_FOCUSED) {
			struct wob_output *output = calloc(1, sizeof(struct wob_output));
			output->wl_output = wl_registry_bind(registry, name, &wl_output_interface, 1);
			output->app = app;
			output->wl_name = name;

			output->xdg_output = zxdg_output_manager_v1_get_xdg_output(app->xdg_output_manager, output->wl_output);
			zxdg_output_v1_add_listener(output->xdg_output, &xdg_output_listener, output);

			if (wl_display_roundtrip(app->wl_display) < 1) {
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

	struct wob_output *output, *output_tmp;
	wl_list_for_each_safe (output, output_tmp, &(app->wob_outputs), link) {
		if (output->wl_name == name) {
			wob_log_info("Output %s disconnected", output->name);
			wob_output_destroy(output);
			wl_list_remove(&output->link);
			free(output);
			return;
		}
	}
}

void
wob_hide(struct wob *app)
{
	struct wob_output *output;
	wl_list_for_each (output, &app->wob_outputs, link) {
		if (output->wob_surface == NULL) continue;
		wob_log_info("Hiding bar on output %s", output->name);
		wob_surface_destroy(output->wob_surface);
		free(output->wob_surface);
		output->wob_surface = NULL;
	}

	if (wl_display_roundtrip(app->wl_display) < 1) {
		wob_log_error("wl_display_roundtrip failed");
		exit(EXIT_FAILURE);
	}
}

void
wob_show(struct wob *app)
{
	struct wob_output *output;
	wl_list_for_each (output, &app->wob_outputs, link) {
		if (output->wob_surface != NULL) continue;
		wob_log_info("Showing bar on output %s", output->name);
		output->wob_surface = wob_surface_create(app, output->wl_output);
	}

	if (wl_display_roundtrip(app->wl_display) < 1) {
		wob_log_error("wl_display_roundtrip failed");
		exit(EXIT_FAILURE);
	}

	wl_list_for_each (output, &(app->wob_outputs), link) {
		wl_surface_attach(output->wob_surface->wl_surface, app->wl_buffer, 0, 0);
		wl_surface_damage(output->wob_surface->wl_surface, 0, 0, INT32_MAX, INT32_MAX);
		wl_surface_commit(output->wob_surface->wl_surface);
	}

	if (wl_display_roundtrip(app->wl_display) < 1) {
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

	wob_config_destroy(&app->wob_config);

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
		.global_remove = handle_global_remove,
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
	if (wl_display_roundtrip(app->wl_display) < 1) {
		wob_log_error("wl_display_roundtrip failed");
		exit(EXIT_FAILURE);
	}

	if (app->wob_config.output_mode == WOB_OUTPUT_MODE_FOCUSED) {
		struct wob_output *output = calloc(1, sizeof(struct wob_output));
		output->wl_output = NULL;
		output->app = app;
		output->wl_name = 0;
		output->xdg_output = NULL;
		output->name = strdup("focused");

		wl_list_insert(&app->wob_outputs, &output->link);
	}

	struct wob_dimensions dimensions = app->wob_config.dimensions;
	struct wl_shm_pool *pool = wl_shm_create_pool(app->wl_shm, app->shmid, dimensions.height * dimensions.width * 4);
	if (pool == NULL) {
		wob_log_error("wl_shm_create_pool failed");
		exit(EXIT_FAILURE);
	}

	app->wl_buffer = wl_shm_pool_create_buffer(pool, 0, dimensions.width, dimensions.height, dimensions.width * 4, WL_SHM_FORMAT_ARGB8888);
	wl_shm_pool_destroy(pool);
	if (app->wl_buffer == NULL) {
		wob_log_error("wl_shm_pool_create_buffer failed");
		exit(EXIT_FAILURE);
	}
}

void
wob_draw(pixman_image_t *image, struct wob_colors colors, struct wob_dimensions dimensions, unsigned long percentage, unsigned long maximum)
{
	pixman_color_t bar_color = wob_color_to_pixman(wob_color_premultiply_alpha(colors.value));
	pixman_color_t background_color = wob_color_to_pixman(wob_color_premultiply_alpha(colors.background));
	pixman_color_t border_color = wob_color_to_pixman(wob_color_premultiply_alpha(colors.border));

	size_t offset_border_padding = dimensions.border_offset + dimensions.border_size + dimensions.bar_padding;
	size_t bar_width = dimensions.width - 2 * offset_border_padding;
	size_t bar_height = dimensions.height - 2 * offset_border_padding;

	uint32_t offset = 0;
	pixman_image_fill_rectangles(PIXMAN_OP_SRC, image, &background_color, 1, &(pixman_rectangle16_t){0, 0, dimensions.width, dimensions.height});
	offset += dimensions.border_offset;
	pixman_image_fill_rectangles(PIXMAN_OP_SRC, image, &border_color, 1, &(pixman_rectangle16_t){offset, offset, dimensions.width - 2 * offset, dimensions.height - 2 * offset});
	offset += dimensions.border_size;
	pixman_image_fill_rectangles(PIXMAN_OP_SRC, image, &background_color, 1, &(pixman_rectangle16_t){offset, offset, dimensions.width - 2 * offset, dimensions.height - 2 * offset});

	offset += dimensions.bar_padding;
	uint32_t filled_width = (bar_width * percentage) / maximum;
	uint32_t filled_height = (bar_height * percentage) / maximum;
	switch (dimensions.orientation) {
		case WOB_ORIENTATION_HORIZONTAL:
			pixman_image_fill_rectangles(PIXMAN_OP_SRC, image, &bar_color, 1, &(pixman_rectangle16_t){offset, offset, filled_width, bar_height});
			break;
		case WOB_ORIENTATION_VERTICAL:
			pixman_image_fill_rectangles(PIXMAN_OP_SRC, image, &bar_color, 1, &(pixman_rectangle16_t){offset, offset + bar_height - filled_height, bar_width, filled_height});
			break;
	}
}

bool
wob_pledge_enabled()
{
	char *disable_pledge_env = getenv("WOB_DISABLE_PLEDGE");
	if (disable_pledge_env != NULL && strcmp(disable_pledge_env, "0") != 0) {
		return false;
	}

	return true;
}

int
main(int argc, char **argv)
{
	wob_log_use_colors(isatty(STDERR_FILENO));
	wob_log_level_warn();

	// libc is doing fstat syscall to determine the optimal buffer size and that can be problematic to wob_pledge()
	// to solve this problem we can just pass the optimal buffer ourselves
	static char stdin_buffer[STDIN_BUFFER_LENGTH];
	if (setvbuf(stdin, stdin_buffer, _IOFBF, sizeof(stdin_buffer)) != 0) {
		wob_log_error("Failed to set stdin buffer size to %zu", sizeof(stdin_buffer));
		return EXIT_FAILURE;
	}
	setvbuf(stdout, NULL, _IONBF, 0);
	setvbuf(stderr, NULL, _IONBF, 0);

	static struct option long_options[] = {
		{"config", required_argument, NULL, 'c'},
		{"help", no_argument, NULL, 'h'},
		{"version", no_argument, NULL, 'V'},
		{"verbose", no_argument, NULL, 'v'},
		{0, 0, 0, 0},
	};

	const char *usage =
		"Usage: wob [options]\n"
		"  -c, --config <config>  Specify a config file.\n"
		"  -v, --verbose          Increase verbosity of messages, defaults to errors and warnings only.\n"
		"  -h, --help             Show help message and quit.\n"
		"  -V, --version          Show the version number and quit.\n"
		"\n";

	int c;
	int option_index = 0;
	char *wob_config_path = NULL;
	while ((c = getopt_long(argc, argv, "hvVc:", long_options, &option_index)) != -1) {
		switch (c) {
			case 'V':
				printf("wob version " WOB_VERSION "\n");
				free(wob_config_path);
				return EXIT_SUCCESS;
			case 'h':
				printf("%s", usage);
				free(wob_config_path);
				return EXIT_SUCCESS;
			case 'v':
				wob_log_inc_verbosity();
				break;
			case 'c':
				// fail if -c option is given multiple times
				if (wob_config_path != NULL) {
					free(wob_config_path);
					fprintf(stderr, "%s", usage);
					return EXIT_FAILURE;
				}

				free(wob_config_path);
				wob_config_path = strdup(optarg);
				break;
			default:
				fprintf(stderr, "%s", usage);
				free(wob_config_path);
				return EXIT_FAILURE;
		}
	}

	wob_log_info("wob version %s started", WOB_VERSION);
	struct wob app = {0};

	if (wob_config_path == NULL) {
		wob_config_path = wob_config_default_path();
	}

	wob_config_init(&app.wob_config);
	if (wob_config_path != NULL) {
		wob_log_info("Using configuration file at %s", wob_config_path);
		if (!wob_config_load(&app.wob_config, wob_config_path)) {
			wob_config_destroy(&app.wob_config);
			free(wob_config_path);
			return EXIT_FAILURE;
		}
	}
	wob_config_debug(&app.wob_config);
	free(wob_config_path);

	int shmid = wob_shm_create();
	if (shmid < 0) {
		return EXIT_FAILURE;
	}
	app.shmid = shmid;

	uint32_t *argb = wob_shm_alloc(shmid, app.wob_config.dimensions.width * app.wob_config.dimensions.height * 4);
	if (argb == NULL) {
		return EXIT_FAILURE;
	}

	wob_connect(&app);
	if (app.wl_shm == NULL || app.wl_compositor == NULL || app.wlr_layer_shell == NULL) {
		wob_log_error("Wayland compositor doesn't support all required protocols");
		return EXIT_FAILURE;
	}

	if (wob_pledge_enabled()) {
		if (!wob_pledge()) {
			return EXIT_FAILURE;
		}
	}

	pixman_image_t *image = pixman_image_create_bits_no_clear(PIXMAN_a8r8g8b8, app.wob_config.dimensions.width, app.wob_config.dimensions.height, argb, app.wob_config.dimensions.width * 4);

	struct wob_colors effective_colors;

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
		char input_buffer[INPUT_BUFFER_LENGTH] = {0};

		switch (poll(fds, 2, hidden ? -1 : app.wob_config.timeout_msec)) {
			case -1:
				wob_log_error("poll() failed: %s", strerror(errno));
				goto exit_cleanup_failure;
			case 0:
				if (!hidden) wob_hide(&app);

				hidden = true;
				break;
			default:
				if (fds[0].revents) {
					if (!(fds[0].revents & POLLIN)) {
						wob_log_error("WL_DISPLAY_FD unexpectedly closed, revents = %hd", fds[0].revents);
						goto exit_cleanup_failure;
					}

					if (wl_display_dispatch(app.wl_display) == -1) {
						goto exit_cleanup_failure;
					}
				}

				if (fds[1].revents) {
					if (!(fds[1].revents & POLLIN)) {
						wob_log_error("STDIN unexpectedly closed, revents = %hd", fds[1].revents);
						goto exit_cleanup_failure;
					}

					char *fgets_rv = fgets(input_buffer, INPUT_BUFFER_LENGTH, stdin);
					if (fgets_rv == NULL) {
						if (feof(stdin)) {
							wob_log_info("Received EOF");
							goto exit_cleanup_success;
						}
						else {
							wob_log_error("fgets() failed: %s", strerror(errno));
							goto exit_cleanup_failure;
						}
					}

					// strip newline from the end of the buffer
					strtok(input_buffer, "\n");

					char *str_end;
					char *token = strtok(input_buffer, " ");
					unsigned long percentage = strtoul(token, &str_end, 10);
					if (*str_end != '\0') {
						wob_log_error("Invalid value received '%s'", token);
						goto exit_cleanup_failure;
					}

					struct wob_style *selected_style = NULL;
					token = strtok(NULL, "");
					if (token != NULL) {
						selected_style = wob_config_find_style(&app.wob_config, token);
						if (selected_style == NULL) {
							wob_log_error("Style named '%s' not found", token);
							goto exit_cleanup_failure;
						}
						wob_log_info("Received input { value = %lu, style = %s }", percentage, token);
					}
					else {
						selected_style = &app.wob_config.default_style;
						wob_log_info("Received input { value = %lu, style = <empty> }", percentage);
					}

					if (percentage > app.wob_config.max) {
						effective_colors = selected_style->overflow_colors;
						switch (app.wob_config.overflow_mode) {
							case WOB_OVERFLOW_MODE_WRAP:
								percentage %= app.wob_config.max;
								break;
							case WOB_OVERFLOW_MODE_NOWRAP:
								percentage = app.wob_config.max;
								break;
						}
					}
					else {
						effective_colors = selected_style->colors;
					}

					if (wl_list_empty(&app.wob_outputs) == 1) {
						wob_log_info("No output found to render wob on");
						break;
					}

					wob_log_info(
						"Rendering { value = %lu, bg = #%08jx, border = #%08jx, bar = #%08jx }",
						percentage,
						wob_color_to_rgba(effective_colors.background),
						wob_color_to_rgba(effective_colors.border),
						wob_color_to_rgba(effective_colors.value)
					);

					wob_draw(image, effective_colors, app.wob_config.dimensions, percentage, app.wob_config.max);
					wob_show(&app);

					hidden = false;
				}
		}
	}

	int _exit_code;
_cleanup:
	if (!hidden) wob_hide(&app);
	wob_destroy(&app);
	pixman_image_unref(image);
	return _exit_code;
exit_cleanup_success:
	_exit_code = EXIT_SUCCESS;
	goto _cleanup;
exit_cleanup_failure:
	_exit_code = EXIT_FAILURE;
	goto _cleanup;
}

// sizeof already includes NULL byte
#define INPUT_BUFFER_LENGTH (3 * sizeof(unsigned long) + sizeof(" #FF000000 #FFFFFFFF #FFFFFFFF\n"))

#define MIN(a, b) (((a) < (b)) ? (a) : (b))

#define _POSIX_C_SOURCE 200809L
#include <errno.h>
#include <poll.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "buffer.h"
#include "options.h"
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
		fprintf(stderr, "strdup failed\n");
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
		fprintf(stderr, "calloc failed\n");
		exit(EXIT_FAILURE);
	}

	wob_surface->wl_surface = wl_compositor_create_surface(app->wl_compositor);
	if (wob_surface->wl_surface == NULL) {
		fprintf(stderr, "wl_compositor_create_surface failed\n");
		exit(EXIT_FAILURE);
	}
	wob_surface->wlr_layer_surface = zwlr_layer_shell_v1_get_layer_surface(app->wlr_layer_shell, wob_surface->wl_surface, wl_output, ZWLR_LAYER_SHELL_V1_LAYER_OVERLAY, "wob");
	if (wob_surface->wlr_layer_surface == NULL) {
		fprintf(stderr, "wlr_layer_shell_v1_get_layer_surface failed\n");
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
				fprintf(stderr, "wl_display_roundtrip failed\n");
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
		fprintf(stderr, "wl_display_dispatch failed\n");
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
		fprintf(stderr, "wl_display_roundtrip failed\n");
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
		fprintf(stderr, "wl_display_roundtrip failed\n");
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
		fprintf(stderr, "wl_display_connect failed\n");
		exit(EXIT_FAILURE);
	}

	app->wl_registry = wl_display_get_registry(app->wl_display);
	if (app->wl_registry == NULL) {
		fprintf(stderr, "wl_display_get_registry failed\n");
		exit(EXIT_FAILURE);
	}

	wl_registry_add_listener(app->wl_registry, &wl_registry_listener, app);

	wl_list_init(&app->wob_outputs);
	if (wl_display_roundtrip(app->wl_display) == -1) {
		fprintf(stderr, "wl_display_roundtrip failed\n");
		exit(EXIT_FAILURE);
	}

	struct wl_shm_pool *pool = wl_shm_create_pool(app->wl_shm, app->shmid, app->wob_geom->size);
	if (pool == NULL) {
		fprintf(stderr, "wl_shm_create_pool failed\n");
		exit(EXIT_FAILURE);
	}

	app->wl_buffer = wl_shm_pool_create_buffer(pool, 0, app->wob_geom->width, app->wob_geom->height, app->wob_geom->stride, WL_SHM_FORMAT_ARGB8888);
	wl_shm_pool_destroy(pool);
	if (app->wl_buffer == NULL) {
		fprintf(stderr, "wl_shm_pool_create_buffer failed\n");
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
	struct wob_options options;
	if (!wob_getopt(argc, argv, &options)) {
		return EXIT_FAILURE;
	}

	struct wob app = {0};
	wl_list_init(&(app.output_configs));

	unsigned long maximum = options.maximum;
	unsigned long timeout_msec = options.timeout_msec;
	struct wob_geom geom = {
		.width = options.bar_width,
		.height = options.bar_height,
		.border_offset = options.border_offset,
		.border_size = options.border_size,
		.bar_padding = options.bar_padding,
		.anchor = options.bar_anchor,
		.margin = options.bar_margin,
	};
	struct wob_colors colors = {
		.background = options.background_color,
		.bar = options.bar_color,
		.border = options.border_color,
	};
	bool pledge = options.pledge;

	struct wob_output_name *output_name, *output_name_tmp;
	struct wob_output_config *output_config;
	wl_list_for_each_safe (output_name, output_name_tmp, &options.outputs, link) {
		output_config = calloc(1, sizeof(struct wob_output_config));
		if (output_config == NULL) {
			fprintf(stderr, "calloc failed\n");
			return EXIT_FAILURE;
		}

		output_config->name = strdup(output_name->name);
		if (output_config->name == NULL) {
			fprintf(stderr, "strdup failed\n");
			return EXIT_FAILURE;
		}

		wl_list_insert(&(app.output_configs), &(output_config->link));
	}

	wob_options_destroy(&options);

	geom.stride = geom.width * 4;
	geom.size = geom.stride * geom.height;
	app.wob_geom = &geom;

	uint32_t *argb = NULL;
	if (!wob_create_argb_buffer(app.wob_geom->size, &(app.shmid), &argb)) {
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
				perror("poll");
				return EXIT_FAILURE;
			case 0:
				if (!hidden) {
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
					wob_destroy(&app);
					return EXIT_SUCCESS;
				}

				old_colors = colors;
				if (fgets_rv == NULL || !wob_parse_input(input_buffer, &percentage, &colors.background, &colors.border, &colors.bar) || percentage > maximum) {
					fprintf(stderr, "Received invalid input\n");
					if (!hidden) {
						wob_hide(&app);
					}
					wob_destroy(&app);
					return EXIT_FAILURE;
				}

				if (hidden) {
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

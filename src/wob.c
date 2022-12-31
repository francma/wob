#define WOB_FILE "wob.c"

#define _POSIX_C_SOURCE 200809L

#include <errno.h>
#include <poll.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "image.h"
#include "log.h"
#include "pledge.h"
#include "wlr-layer-shell-unstable-v1-client-protocol.h"
#include "wob.h"

struct wob_surface {
	struct zwlr_layer_surface_v1 *wlr_layer_surface;
	struct wl_surface *wl_surface;
};

struct wob_output {
	char *name;
	struct wl_list link;
	struct wl_output *wl_output;
	struct wob_surface *wob_surface;
	uint32_t wl_name;
	bool focused;
};

struct wob {
	struct wob_image *image;
	struct wl_buffer *wl_buffer;
	struct wl_compositor *wl_compositor;
	struct wl_display *wl_display;
	struct wl_list wob_outputs;
	struct wl_registry *wl_registry;
	struct wl_shm *wl_shm;
	struct zwlr_layer_shell_v1 *wlr_layer_shell;
	struct wob_config *config;
};

void
noop()
{
	/* intentionally left blank */
}

bool
wob_should_render_on_output(struct wob *app, struct wob_output *output)
{
	switch (app->config->output_mode) {
		case WOB_OUTPUT_MODE_FOCUSED:
			return output->focused;
		case WOB_OUTPUT_MODE_ALL:
			return true;
		case WOB_OUTPUT_MODE_WHITELIST:
			return wob_config_find_output_by_name(app->config, output->name) != NULL;
	}

	return false;
}

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

void
layer_surface_configure(void *data, struct zwlr_layer_surface_v1 *surface, uint32_t serial, uint32_t w, uint32_t h)
{
	zwlr_layer_surface_v1_ack_configure(surface, serial);
}

struct wob_surface *
wob_surface_create(struct wob *app, struct wl_output *wl_output)
{
	struct wob_config config = *app->config;
	const static struct zwlr_layer_surface_v1_listener zwlr_layer_surface_listener = {
		.configure = layer_surface_configure,
		.closed = noop,
	};

	struct wob_surface *wob_surface = calloc(1, sizeof(struct wob_surface));
	if (wob_surface == NULL) {
		wob_log_panic("calloc failed");
	}

	wob_surface->wl_surface = wl_compositor_create_surface(app->wl_compositor);
	if (wob_surface->wl_surface == NULL) {
		wob_log_panic("wl_compositor_create_surface failed");
	}
	wob_surface->wlr_layer_surface = zwlr_layer_shell_v1_get_layer_surface(app->wlr_layer_shell, wob_surface->wl_surface, wl_output, ZWLR_LAYER_SHELL_V1_LAYER_OVERLAY, "wob");
	if (wob_surface->wlr_layer_surface == NULL) {
		wob_log_panic("wlr_layer_shell_v1_get_layer_surface failed");
	}

	struct wob_dimensions dimensions = config.dimensions;
	struct wob_margin margin = config.margin;
	enum wob_anchor anchor = config.anchor;

	zwlr_layer_surface_v1_set_size(wob_surface->wlr_layer_surface, dimensions.width, dimensions.height);
	zwlr_layer_surface_v1_set_anchor(wob_surface->wlr_layer_surface, wob_anchor_to_wlr_layer_surface_anchor(anchor));
	zwlr_layer_surface_v1_set_margin(wob_surface->wlr_layer_surface, margin.top, margin.right, margin.bottom, margin.left);
	zwlr_layer_surface_v1_add_listener(wob_surface->wlr_layer_surface, &zwlr_layer_surface_listener, app);

	return wob_surface;
}

void
wob_surface_destroy(struct wob_surface *wob_surface)
{
	zwlr_layer_surface_v1_destroy(wob_surface->wlr_layer_surface);
	wl_surface_destroy(wob_surface->wl_surface);

	free(wob_surface);
}

void
wob_output_destroy(struct wob_output *output)
{
	if (output->wob_surface != NULL) {
		wob_surface_destroy(output->wob_surface);
	}

	if (output->wl_output != NULL) {
		wl_output_destroy(output->wl_output);
	}

	free(output->name);
	free(output);
}

void
xdg_output_handle_name(void *data, struct wl_output *wl_output, const char *name)
{
	struct wob_output *output = (struct wob_output *) data;
	free(output->name);

	output->name = strdup(name);
	if (output->name == NULL) {
		wob_log_panic("strdup failed\n");
	}
}

void
xdg_output_handle_done(void *data, struct wl_output *wl_output)
{
	struct wob_output *output = (struct wob_output *) data;

	wob_log_debug("Detected new output %s", output->name);
}

void
handle_global(void *data, struct wl_registry *registry, uint32_t name, const char *interface, uint32_t version)
{
	const static struct wl_output_listener wl_output_listener = {
		.geometry = noop,
		.mode = noop,
		.scale = noop,
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
	else if (strcmp(interface, wl_output_interface.name) == 0) {
		struct wob_output *output = calloc(1, sizeof(struct wob_output));
		output->wl_name = name;
		output->name = strdup("UNKNOWN");

		if (version < 4) {
			wob_log_warn("Need %s version > 4 to match outputs based on name, got version %zu. Some features might not work.", wl_output_interface.name, version);
			output->wl_output = wl_registry_bind(registry, name, &wl_output_interface, version);
		}
		else {
			output->wl_output = wl_registry_bind(registry, name, &wl_output_interface, 4);
		}

		wl_list_insert(&app->wob_outputs, &output->link);
		wl_output_add_listener(output->wl_output, &wl_output_listener, output);
	}
	else if (strcmp(interface, zwlr_layer_shell_v1_interface.name) == 0) {
		app->wlr_layer_shell = wl_registry_bind(registry, name, &zwlr_layer_shell_v1_interface, 1);
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
			wl_list_remove(&output->link);
			wob_output_destroy(output);
			return;
		}
	}
}

int
wob_run(struct wob_config *config)
{
	struct wob *app = calloc(1, sizeof(struct wob));
	struct wob_image *image = wob_image_create_argb8888(config->dimensions.width, config->dimensions.height);

	app->image = image;
	app->config = config;

	wl_list_init(&app->wob_outputs);
	if (app->config->output_mode == WOB_OUTPUT_MODE_FOCUSED) {
		struct wob_output *output = calloc(1, sizeof(struct wob_output));
		output->wl_output = NULL;
		output->wl_name = 0;
		output->focused = true;
		output->name = strdup("focused");

		wl_list_insert(&app->wob_outputs, &output->link);
	}

	const static struct wl_registry_listener wl_registry_listener = {
		.global = handle_global,
		.global_remove = handle_global_remove,
	};

	app->wl_display = wl_display_connect(NULL);
	if (app->wl_display == NULL) {
		wob_log_panic("wl_display_connect failed");
	}

	if (config->sandbox) {
		wob_pledge();
	}

	app->wl_registry = wl_display_get_registry(app->wl_display);
	if (app->wl_registry == NULL) {
		wob_log_panic("wl_display_get_registry failed");
	}

	wl_registry_add_listener(app->wl_registry, &wl_registry_listener, app);
	// first roundtrip to get global interfaces
	if (wl_display_roundtrip(app->wl_display) < 0) {
		wob_log_panic("global listeners wl_display_roundtrip failed");
	}
	// second roundtrip for output configuration
	if (wl_display_roundtrip(app->wl_display) < 0) {
		wob_log_panic("global listeners wl_display_roundtrip failed");
	}

	if (app->wl_shm == NULL || app->wl_compositor == NULL || app->wlr_layer_shell == NULL) {
		wob_log_panic("Wayland compositor doesn't support all required protocols");
	}

	struct wl_shm_pool *pool = wl_shm_create_pool(app->wl_shm, app->image->shmid, app->image->size_in_bytes);
	if (pool == NULL) {
		wob_log_panic("wl_shm_create_pool failed");
	}

	app->wl_buffer = wl_shm_pool_create_buffer(pool, 0, app->image->width, app->image->height, app->image->stride, WL_SHM_FORMAT_ARGB8888);
	wl_shm_pool_destroy(pool);
	if (app->wl_buffer == NULL) {
		wob_log_panic("wl_shm_pool_create_buffer failed");
	}

	struct wob_colors effective_colors;

	struct pollfd fds[2] = {
		{
			.fd = wl_display_get_fd(app->wl_display),
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

		switch (poll(fds, 2, hidden ? -1 : app->config->timeout_msec)) {
			case -1:
				wob_log_panic("poll() failed: %s", strerror(errno));
			case 0:
				if (!hidden) {
					struct wob_output *output;
					wl_list_for_each (output, &app->wob_outputs, link) {
						if (output->wob_surface == NULL) continue;
						wob_log_info("Hiding bar on output %s", output->name);
						wob_surface_destroy(output->wob_surface);
						output->wob_surface = NULL;
					}

					if (wl_display_roundtrip(app->wl_display) < 1) {
						wob_log_panic("wl_display_roundtrip failed");
					}
				}

				hidden = true;
				break;
			default:
				if (fds[0].revents) {
					if (!(fds[0].revents & POLLIN)) {
						wob_log_panic("WL_DISPLAY_FD unexpectedly closed, revents = %hd", fds[0].revents);
					}

					if (wl_display_dispatch(app->wl_display) == -1) {
						wob_log_panic("wl_display_dispatch failed");
					}

					if (wl_display_roundtrip(app->wl_display) < 0) {
						wob_log_panic("wl_display_roundtrip failed");
					}
				}

				if (fds[1].revents) {
					if (!(fds[1].revents & POLLIN)) {
						wob_log_error("STDIN unexpectedly closed, revents = %hd", fds[1].revents);
						goto exit_failure;
					}

					char *fgets_rv = fgets(input_buffer, INPUT_BUFFER_LENGTH, stdin);
					if (fgets_rv == NULL) {
						if (feof(stdin)) {
							wob_log_info("Received EOF");
							goto exit_success;
						}
						else {
							wob_log_error("fgets() failed: %s", strerror(errno));
							goto exit_failure;
						}
					}

					// strip newline from the end of the buffer
					strtok(input_buffer, "\n");

					char *str_end;
					char *token = strtok(input_buffer, " ");
					unsigned long percentage = strtoul(token, &str_end, 10);
					if (*str_end != '\0') {
						wob_log_error("Invalid value received '%s'", token);
						goto exit_failure;
					}

					struct wob_style *selected_style = NULL;
					token = strtok(NULL, "");
					if (token != NULL) {
						selected_style = wob_config_find_style(app->config, token);
						if (selected_style == NULL) {
							wob_log_error("Style named '%s' not found", token);
							goto exit_failure;
						}
						wob_log_info("Received input { value = %lu, style = %s }", percentage, token);
					}
					else {
						selected_style = &app->config->default_style;
						wob_log_info("Received input { value = %lu, style = <empty> }", percentage);
					}

					if (percentage > app->config->max) {
						effective_colors = selected_style->overflow_colors;
						switch (app->config->overflow_mode) {
							case WOB_OVERFLOW_MODE_WRAP:
								percentage %= app->config->max;
								break;
							case WOB_OVERFLOW_MODE_NOWRAP:
								percentage = app->config->max;
								break;
						}
					}
					else {
						effective_colors = selected_style->colors;
					}

					if (wl_list_empty(&app->wob_outputs) == 1) {
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

					wob_image_draw(app->image, effective_colors, app->config->dimensions, percentage, app->config->max);

					struct wob_output *output;
					wl_list_for_each (output, &app->wob_outputs, link) {
						if (output->wob_surface != NULL) {
							continue;
						}

						if (!wob_should_render_on_output(app, output)) {
							wob_log_info("NOT Showing bar on output %s", output->name);
							continue;
						}

						output->wob_surface = wob_surface_create(app, output->wl_output);
						wl_surface_commit(output->wob_surface->wl_surface);
						wob_log_info("Showing bar on output %s", output->name);
					}

					if (wl_display_roundtrip(app->wl_display) < 0) {
						wob_log_panic("wl_display_roundtrip failed");
					}

					wl_list_for_each (output, &(app->wob_outputs), link) {
						if (output->wob_surface == NULL) {
							continue;
						}
						wl_surface_attach(output->wob_surface->wl_surface, app->wl_buffer, 0, 0);
						wl_surface_damage(output->wob_surface->wl_surface, 0, 0, INT32_MAX, INT32_MAX);
						wl_surface_commit(output->wob_surface->wl_surface);
					}

					if (wl_display_roundtrip(app->wl_display) < 1) {
						wob_log_panic("wl_display_roundtrip failed");
					}

					hidden = false;
				}
		}
	}

	int _exit_code;
_exit_cleanup : {
	struct wob_output *output, *output_tmp;
	wl_list_for_each_safe (output, output_tmp, &app->wob_outputs, link) {
		wob_output_destroy(output);
	}

	zwlr_layer_shell_v1_destroy(app->wlr_layer_shell);
	wl_buffer_destroy(app->wl_buffer);
	wl_compositor_destroy(app->wl_compositor);
	wl_shm_destroy(app->wl_shm);
	wl_registry_destroy(app->wl_registry);

	wl_display_roundtrip(app->wl_display);

	wl_display_disconnect(app->wl_display);

	wob_config_destroy(app->config);
	wob_image_destroy(app->image);
	free(app);
}
	return _exit_code;
exit_success:
	_exit_code = EXIT_SUCCESS;
	goto _exit_cleanup;
exit_failure:
	_exit_code = EXIT_FAILURE;
	goto _exit_cleanup;
}

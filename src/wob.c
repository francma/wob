#define WOB_FILE "wob.c"

#include <errno.h>
#include <poll.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>
#include <wayland-client-protocol.h>

#include "fractional-scale-v1.h"
#include "image.h"
#include "log.h"
#include "pledge.h"
#include "shm.h"
#include "viewporter.h"
#include "wlr-layer-shell-unstable-v1.h"
#include "wob.h"

struct wob_buffer {
	struct wl_buffer *wl_buffer;
	struct wob_dimensions dimensions;
	uint32_t *shm_data;
};

struct wob_surface {
	struct zwlr_layer_surface_v1 *wlr_layer_surface;
	struct wl_surface *wl_surface;
	struct wp_fractional_scale_v1 *fractional;
	struct wp_viewport *wp_viewport;

	struct wob_dimensions dimensions;
	struct wob_margin margin;
	enum wob_anchor anchor;
	struct wob_buffer *wob_buffer;
	uint32_t scale;

	// TODO move somewhere?
	double desired_percentage;
	struct wob_colors desired_colors;
};

struct wob_output {
	char *name;
	char *description;
	struct wl_list link;
	struct wl_output *wl_output;
	uint32_t wl_name;
};

struct wob {
	struct wl_list wob_outputs;
	struct wob_config *config;
	struct wob_surface *surface;
	int shmid;
};

struct managers {
	struct wl_compositor *wl_compositor;
	struct wp_fractional_scale_manager_v1 *wp_fractional_scale;
	struct zwlr_layer_shell_v1 *wlr_layer_shell;
	struct wp_viewporter *wp_viewporter;
	struct wl_shm *wl_shm;
};
static struct managers managers;

void
noop()
{
	/* intentionally left blank */
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

static struct wl_callback_listener wl_surface_frame_listener;

struct wob_buffer *
wob_buffer_create_argb8888(int shmid, const struct wob_dimensions dimensions)
{
	size_t width = dimensions.width;
	size_t height = dimensions.height;
	size_t shm_size = width * height * 4;

	void *shm_data = wob_shm_allocate(shmid, shm_size);
	if (shm_data == NULL) {
		wob_log_panic("wob_shm_allocate() failed");
	}

	struct wl_shm_pool *pool = wl_shm_create_pool(managers.wl_shm, shmid, shm_size);
	if (pool == NULL) {
		wob_log_panic("wl_shm_create_pool failed");
	}

	struct wl_buffer *wl_buffer = wl_shm_pool_create_buffer(pool, 0, width, height, width * 4, WL_SHM_FORMAT_ARGB8888);
	if (wl_buffer == NULL) {
		wob_log_panic("wl_shm_pool_create_buffer failed");
	}
	wl_shm_pool_destroy(pool);

	struct wob_buffer *wob_buffer = calloc(1, sizeof(struct wob_buffer));
	if (wob_buffer == NULL) {
		wob_log_panic("calloc failed");
	}

	*wob_buffer = (struct wob_buffer) {
		.wl_buffer = wl_buffer,
		.dimensions = dimensions,
		.shm_data = shm_data,
	};

	wob_log_debug("created buffer %zu x %zu", width, height);

	return wob_buffer;
}

void
wob_buffer_destroy(struct wob_buffer *buffer)
{
	wl_buffer_destroy(buffer->wl_buffer);
	if (buffer->shm_data != NULL) {
		munmap(buffer->shm_data, buffer->dimensions.width * buffer->dimensions.height * 4);
	}
	free(buffer);
}

void
layer_surface_configure(void *data, struct zwlr_layer_surface_v1 *zwlr_surface, uint32_t serial, uint32_t w, uint32_t h)
{
	struct wob *state = data;

	wob_log_debug("layer_surface_configure(%p, %u, %u, %u)", (void *) zwlr_surface, (uintmax_t) serial, w, h);

	zwlr_layer_surface_v1_ack_configure(zwlr_surface, serial);

	int shmid = state->shmid;
	struct wob_surface *surface = state->surface;
	if (surface == NULL) {
		wob_log_panic("surface is NULL");
	}

	struct wob_dimensions scaled_dimensions = wob_dimensions_apply_scale(surface->dimensions, surface->scale);
	if (surface->wob_buffer == NULL || !wob_dimensions_eq(surface->wob_buffer->dimensions, scaled_dimensions)) {
		if (surface->wob_buffer != NULL) {
			wob_buffer_destroy(surface->wob_buffer);
		}
		surface->wob_buffer = wob_buffer_create_argb8888(shmid, scaled_dimensions);

		// redraw only if we have dimensions set, otherwise keep the transparent pixel
		if (surface->dimensions.height != 1 || surface->dimensions.width != 1) {
			wob_image_draw(surface->wob_buffer->shm_data, surface->wob_buffer->dimensions, surface->desired_colors, surface->desired_percentage);
		}

		if (surface->wp_viewport != NULL) {
			wp_viewport_set_destination(surface->wp_viewport, surface->dimensions.width, surface->dimensions.height);
		}

		wl_surface_attach(surface->wl_surface, surface->wob_buffer->wl_buffer, 0, 0);
		wl_surface_damage_buffer(surface->wl_surface, 0, 0, INT32_MAX, INT32_MAX);
		wl_surface_commit(surface->wl_surface);
	}
}

void
layer_surface_enter(void *data, struct wl_surface *wl_surface, struct wl_output *entered_output)
{
	wob_log_debug("layer_surface_enter(%x)", wl_surface);
	struct wob *app = data;

	struct wob_output *selected_output;
	wl_list_for_each (selected_output, &app->wob_outputs, link) {
		if (entered_output == selected_output->wl_output) {
			break;
		}
	}

	// defaults
	struct wob_margin margin = app->config->margin;
	struct wob_dimensions dimensions = app->config->dimensions;
	enum wob_anchor anchor = app->config->anchor;

	// try to match output config
	struct wob_output_config *output_config = NULL;
	if (selected_output->name != NULL) {
		output_config = wob_config_match_output(app->config, selected_output->name);
	}
	if (output_config != NULL && selected_output->description != NULL) {
		output_config = wob_config_match_output(app->config, selected_output->description);
	}

	if (output_config != NULL) {
		margin = output_config->margin;
		dimensions = output_config->dimensions;
		anchor = output_config->anchor;
	}

	struct wob_surface *surface = app->surface;
	if (!wob_dimensions_eq(surface->dimensions, dimensions) || !wob_margin_eq(margin, surface->margin) || anchor != surface->anchor) {
		zwlr_layer_surface_v1_set_anchor(surface->wlr_layer_surface, wob_anchor_to_wlr_layer_surface_anchor(anchor));
		zwlr_layer_surface_v1_set_margin(surface->wlr_layer_surface, margin.top, margin.right, margin.bottom, margin.left);
		zwlr_layer_surface_v1_set_size(surface->wlr_layer_surface, dimensions.width, dimensions.height);

		surface->dimensions = dimensions;
		wl_surface_commit(surface->wl_surface);
	}

	// no need to redraw, wait for configure event
}

void
wp_fractional_scale_preferred_scale(void *data, struct wp_fractional_scale_v1 *wp_fractional_scale, uint32_t scale)
{
	(void) wp_fractional_scale;

	struct wob *app = data;

	struct wob_surface *surface = app->surface;

	wob_log_debug("setting fractional scale to %u", scale);
	surface->scale = scale;
}

struct wob_surface *
wob_create_surface(struct wob *app)
{
	static const struct zwlr_layer_surface_v1_listener zwlr_layer_surface_listener = {
		.configure = layer_surface_configure,
		.closed = noop,
	};

	static const struct wl_surface_listener wl_surface_listener = {
		.enter = layer_surface_enter,
		.leave = noop,
		.preferred_buffer_scale = noop,
		.preferred_buffer_transform = noop,
	};

	struct wob_dimensions dimensions = {
		.width = 1,
		.height = 1,
		.bar_padding = 0,
		.border_size = 0,
		.border_offset = 0,
		.orientation = WOB_ORIENTATION_HORIZONTAL,
	};

	struct wob_margin margin = {.top = 0, .right = 0, .bottom = 0, .left = 0};

	struct wl_surface *wl_surface = wl_compositor_create_surface(managers.wl_compositor);
	if (wl_surface == NULL) {
		wob_log_panic("wl_compositor_create_surface failed");
	}
	wl_surface_add_listener(wl_surface, &wl_surface_listener, app);

	struct zwlr_layer_surface_v1 *wlr_layer_surface = zwlr_layer_shell_v1_get_layer_surface(managers.wlr_layer_shell, wl_surface, NULL, ZWLR_LAYER_SHELL_V1_LAYER_OVERLAY, "wob");
	if (wlr_layer_surface == NULL) {
		wob_log_panic("wlr_layer_shell_v1_get_layer_surface failed");
	}
	zwlr_layer_surface_v1_set_size(wlr_layer_surface, dimensions.width, dimensions.height);
	zwlr_layer_surface_v1_add_listener(wlr_layer_surface, &zwlr_layer_surface_listener, app);

	struct wp_viewport *wp_viewport = NULL;
	if (managers.wp_viewporter != NULL) {
		wp_viewport = wp_viewporter_get_viewport(managers.wp_viewporter, wl_surface);
		if (wp_viewport == NULL) {
			wob_log_panic("wp_viewporter_get_viewport failed");
		}
	}

	static struct wp_fractional_scale_v1_listener wp_fractional_scale_listener = {
		.preferred_scale = wp_fractional_scale_preferred_scale,
	};

	struct wp_fractional_scale_v1 *wp_fractional = NULL;
	if (managers.wp_fractional_scale != NULL && wp_viewport != NULL) {
		wp_fractional = wp_fractional_scale_manager_v1_get_fractional_scale(managers.wp_fractional_scale, wl_surface);
		if (wp_fractional == NULL) {
			wob_log_panic("wp_fractional_scale_manager_v1_get_fractional_scale failed");
		}

		wp_fractional_scale_v1_add_listener(wp_fractional, &wp_fractional_scale_listener, app);
	}

	struct wob_surface *rendered = calloc(1, sizeof(struct wob_surface));
	if (rendered == NULL) {
		wob_log_panic("calloc failed");
	}

	*rendered = (struct wob_surface) {
		.wlr_layer_surface = wlr_layer_surface,
		.wl_surface = wl_surface,
		.dimensions = dimensions,
		.scale = 120,
		.wob_buffer = NULL,
		.margin = margin,
		.anchor = 0,
		.wp_viewport = wp_viewport,
		.fractional = wp_fractional,
	};

	wl_surface_commit(wl_surface);

	return rendered;
}

void
wl_surface_frame_done(void *data, struct wl_callback *cb, uint32_t time)
{
	(void) time;

	wl_callback_destroy(cb);

	struct wob_surface *surface = data;
	wob_log_debug("rendering frame");

	wob_image_draw(surface->wob_buffer->shm_data, surface->wob_buffer->dimensions, surface->desired_colors, surface->desired_percentage);

	wl_surface_attach(surface->wl_surface, surface->wob_buffer->wl_buffer, 0, 0);
	wl_surface_damage_buffer(surface->wl_surface, 0, 0, INT32_MAX, INT32_MAX);
	wl_surface_commit(surface->wl_surface);
}

void
wob_surface_destroy(struct wob_surface *wob_surface)
{
	zwlr_layer_surface_v1_destroy(wob_surface->wlr_layer_surface);
	wl_surface_destroy(wob_surface->wl_surface);

	if (wob_surface->wp_viewport != NULL) {
		wp_viewport_destroy(wob_surface->wp_viewport);
	}
	if (wob_surface->fractional != NULL) {
		wp_fractional_scale_v1_destroy(wob_surface->fractional);
	}
	if (wob_surface->wob_buffer != NULL) {
		wob_buffer_destroy(wob_surface->wob_buffer);
	}

	free(wob_surface);
}

void
wob_output_destroy(struct wob_output *output)
{
	if (output->wl_output != NULL) {
		wl_output_destroy(output->wl_output);
	}

	free(output->description);
	free(output->name);
	free(output);
}

void
xdg_output_handle_name(void *data, struct wl_output *wl_output, const char *name)
{
	(void) wl_output;

	struct wob_output *output = data;
	free(output->name);

	output->name = strdup(name);
	if (output->name == NULL) {
		wob_log_panic("strdup failed");
	}
}

void
xdg_output_handle_description(void *data, struct wl_output *wl_output, const char *description)
{
	(void) wl_output;

	struct wob_output *output = data;

	free(output->description);

	output->description = strdup(description);
	if (output->description == NULL) {
		wob_log_panic("strdup failed");
	}
}

void
xdg_output_handle_geometry(
	void *data, struct wl_output *wl_output, int32_t x, int32_t y, int32_t physical_width, int32_t physical_height, int32_t subpixel, const char *make, const char *model, int32_t transform
)
{
	(void) wl_output;
	(void) x;
	(void) y;
	(void) physical_height;
	(void) physical_width;
	(void) subpixel;
	(void) transform;

	struct wob_output *output = data;

	if (strcmp(output->description, "UNKNOWN") == 0) {
		free(output->description);

		size_t size = strlen(make) + strlen(model) + 1 + 1; // NULL BYTE + ' '
		output->description = malloc(size);
		if (output->description == NULL) {
			wob_log_panic("malloc failed");
		}

		snprintf(output->description, size, "%s %s", make, model);
	}
}

void
xdg_output_handle_done(void *data, struct wl_output *wl_output)
{
	(void) wl_output;

	struct wob_output *output = data;

	wob_log_debug("Detected new output name = %s, description = %s", output->name, output->description);
}

void
handle_global(void *data, struct wl_registry *registry, uint32_t name, const char *interface, uint32_t version)
{
	static const struct wl_output_listener wl_output_listener = {
		.geometry = xdg_output_handle_geometry,
		.mode = noop,
		.scale = noop,
		.name = xdg_output_handle_name,
		.description = xdg_output_handle_description,
		.done = xdg_output_handle_done,
	};

	struct wob *app = data;

	if (strcmp(interface, wl_shm_interface.name) == 0) {
		managers.wl_shm = wl_registry_bind(registry, name, &wl_shm_interface, 1);
	}
	else if (strcmp(interface, wl_compositor_interface.name) == 0) {
		managers.wl_compositor = wl_registry_bind(registry, name, &wl_compositor_interface, 4);
	}
	else if (strcmp(interface, wl_output_interface.name) == 0) {
		struct wob_output *output = calloc(1, sizeof(struct wob_output));
		output->wl_name = name;
		output->name = strdup("UNKNOWN");
		output->description = strdup("UNKNOWN");

		if (version < 4) {
			wob_log_warn("Need %s version > 4 to match outputs based on name & description, got version %zu. Output matching will not work.", wl_output_interface.name, version);
			output->wl_output = wl_registry_bind(registry, name, &wl_output_interface, version);
		}
		else {
			output->wl_output = wl_registry_bind(registry, name, &wl_output_interface, 4);
		}

		wl_list_insert(&app->wob_outputs, &output->link);
		wl_output_add_listener(output->wl_output, &wl_output_listener, output);
	}
	else if (strcmp(interface, zwlr_layer_shell_v1_interface.name) == 0) {
		managers.wlr_layer_shell = wl_registry_bind(registry, name, &zwlr_layer_shell_v1_interface, 1);
	}
	else if (strcmp(interface, wp_viewporter_interface.name) == 0) {
		managers.wp_viewporter = wl_registry_bind(registry, name, &wp_viewporter_interface, 1);
	}
	else if (strcmp(interface, wp_fractional_scale_manager_v1_interface.name) == 0) {
		managers.wp_fractional_scale = wl_registry_bind(registry, name, &wp_fractional_scale_manager_v1_interface, 1);
	}
}

void
handle_global_remove(void *data, struct wl_registry *registry, uint32_t name)
{
	(void) registry;

	struct wob *app = data;

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
	int _exit_code;

	wl_surface_frame_listener.done = &wl_surface_frame_done;

	struct wob *state = calloc(1, sizeof(struct wob));

	state->shmid = wob_shm_open();

	state->config = config;
	wl_list_init(&state->wob_outputs);

	static const struct wl_registry_listener wl_registry_listener = {
		.global = handle_global,
		.global_remove = handle_global_remove,
	};

	struct wl_display *wl_display = wl_display_connect(NULL);
	if (wl_display == NULL) {
		wob_log_panic("wl_display_connect failed");
	}

	if (config->sandbox) {
		wob_pledge();
	}

	struct wl_registry *wl_registry = wl_display_get_registry(wl_display);
	if (wl_registry == NULL) {
		wob_log_panic("wl_display_get_registry failed");
	}

	wl_registry_add_listener(wl_registry, &wl_registry_listener, state);
	// first roundtrip to get global interfaces
	if (wl_display_roundtrip(wl_display) < 0) {
		wob_log_panic("global listeners wl_display_roundtrip failed");
	}
	// second roundtrip for output configuration
	if (wl_display_roundtrip(wl_display) < 0) {
		wob_log_panic("global listeners wl_display_roundtrip failed");
	}

	if (managers.wl_shm == NULL || managers.wl_compositor == NULL || managers.wlr_layer_shell == NULL) {
		wob_log_panic("Wayland compositor doesn't support all required protocols");
	}

	struct wob_colors effective_colors;

	struct pollfd fds[2] = {
		{
			.fd = wl_display_get_fd(wl_display),
			.events = POLLIN,
		},
		{
			.fd = STDIN_FILENO,
			.events = POLLIN,
		},
	};

	for (;;) {
		char input_buffer[INPUT_BUFFER_LENGTH] = {0};

		int timeout = -1;
		if (state->surface != NULL) {
			timeout = state->config->timeout_msec;
		}

		switch (poll(fds, 2, timeout)) {
			case -1:
				wob_log_panic("poll() failed: %s", strerror(errno));
			case 0:
				if (state->surface != NULL) {
					wob_log_info("Hiding bar");
					wob_surface_destroy(state->surface);
					state->surface = NULL;

					wl_display_flush(wl_display);
				}

				break;
			default:
				if (fds[0].revents) {
					if (!(fds[0].revents & POLLIN)) {
						wob_log_panic("WL_DISPLAY_FD unexpectedly closed, revents = %hd", fds[0].revents);
					}

					wob_log_debug("read");
					if (wl_display_dispatch(wl_display) == -1) {
						wob_log_panic("wl_display_dispatch failed");
					}

					wl_display_flush(wl_display);
				}

				if (fds[1].revents) {
					if (!(fds[1].revents & POLLIN)) {
						wob_log_error("STDIN unexpectedly closed, revents = %hd", fds[1].revents);
						_exit_code = EXIT_FAILURE;
						goto _exit_cleanup;
					}

					char *fgets_rv = fgets(input_buffer, INPUT_BUFFER_LENGTH, stdin);
					if (fgets_rv == NULL) {
						if (feof(stdin)) {
							wob_log_info("Received EOF");
							_exit_code = EXIT_SUCCESS;
						}
						else {
							wob_log_error("fgets() failed: %s", strerror(errno));
							_exit_code = EXIT_FAILURE;
						}
						goto _exit_cleanup;
					}

					// strip newline from the end of the buffer
					strtok(input_buffer, "\n");

					char *str_end;
					char *token = strtok(input_buffer, " ");
					unsigned long percentage = strtoul(token, &str_end, 10);
					if (*str_end != '\0') {
						wob_log_warn("Invalid value received '%s'", token);
						break;
					}

					struct wob_style *selected_style = &state->config->default_style;
					token = strtok(NULL, "");
					wob_log_info("Received input { value = %lu, style = %s }", percentage, token != NULL ? token : "<empty>");
					if (token != NULL) {
						struct wob_style *selected_style_search = wob_config_find_style(state->config, token);
						if (selected_style_search != NULL) {
							selected_style = selected_style_search;
						}
						else {
							wob_log_warn("Style named '%s' not found, using the default one", token);
						}
					}

					if (percentage > state->config->max) {
						effective_colors = selected_style->overflow_colors;
						switch (state->config->overflow_mode) {
							case WOB_OVERFLOW_MODE_WRAP:
								percentage %= state->config->max;
								break;
							case WOB_OVERFLOW_MODE_NOWRAP:
								percentage = state->config->max;
								break;
						}
					}
					else {
						effective_colors = selected_style->colors;
					}

					if (wl_list_empty(&state->wob_outputs)) {
						wob_log_info("No output found to render wob on");
						break;
					}

					wob_log_info(
						"Rendering { value = %lu, bg = " WOB_COLOR_PRINTF_FORMAT ", border = " WOB_COLOR_PRINTF_FORMAT ", bar = " WOB_COLOR_PRINTF_FORMAT " }",
						percentage,
						WOB_COLOR_PRINTF_RGBA(effective_colors.background),
						WOB_COLOR_PRINTF_RGBA(effective_colors.border),
						WOB_COLOR_PRINTF_RGBA(effective_colors.value)
					);

					if (state->surface == NULL) {
						state->surface = wob_create_surface(state);
					}
					else {
						struct wl_callback *cb = wl_surface_frame(state->surface->wl_surface);
						wl_callback_add_listener(cb, &wl_surface_frame_listener, state->surface);
						wl_surface_commit(state->surface->wl_surface);
					}

					state->surface->desired_colors = effective_colors;
					state->surface->desired_percentage = (double) percentage / (double) state->config->max;

					wl_display_flush(wl_display);
				}
		}
	}

_exit_cleanup:
	// cleanup state
	if (state->surface != NULL) {
		wob_surface_destroy(state->surface);
	}
	struct wob_output *output, *output_tmp;
	wl_list_for_each_safe (output, output_tmp, &state->wob_outputs, link) {
		wob_output_destroy(output);
	}
	wob_config_destroy(state->config);
	free(state);

	// cleanup global managers & registry
	zwlr_layer_shell_v1_destroy(managers.wlr_layer_shell);
	wl_compositor_destroy(managers.wl_compositor);
	wl_shm_destroy(managers.wl_shm);
	if (managers.wp_viewporter != NULL) {
		wp_viewporter_destroy(managers.wp_viewporter);
	}
	if (managers.wp_fractional_scale != NULL) {
		wp_fractional_scale_manager_v1_destroy(managers.wp_fractional_scale);
	}
	wl_registry_destroy(wl_registry);

	// roundtrip and disconnect
	wl_display_roundtrip(wl_display);
	wl_display_disconnect(wl_display);

	return _exit_code;
}

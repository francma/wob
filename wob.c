#define WIDTH 400
#define HEIGHT 50
#define BORDER_OFFSET 4
#define BORDER_SIZE 4

#define BLACK 0xFF000000
#define WHITE 0xFFFFFFFF

#define STRIDE WIDTH * 4
#define SIZE STRIDE * HEIGHT

#define _POSIX_C_SOURCE 200809L
#ifndef DEBUG
#define NDEBUG
#endif
#include <assert.h>
#include <fcntl.h>    // shm
#include <stdbool.h>  // true, false
#include <stdio.h>    // NULL
#include <stdlib.h>   // EXIT_FAILURE
#include <string.h>   // strcmp
#include <sys/mman.h> // shm
#include <time.h>   // nanosleep
#include <unistd.h> // shm, ftruncate
#include <poll.h>

#include "wlr-layer-shell-unstable-v1.h"
#include "xdg-shell-client-protocol.h"

struct wob {
	struct wl_buffer *wl_buffer;
	struct wl_compositor *wl_compositor;
	struct wl_display *wl_display;
	struct wl_output *wl_output;
	struct wl_registry *wl_registry;
	struct wl_shm *wl_shm;
	struct wl_surface *wl_surface;
	struct xdg_wm_base *xdg_wm_base;
	struct zwlr_layer_shell_v1 *zwlr_layer_shell;
	struct zwlr_layer_surface_v1 *zwlr_layer_surface;
	int shmid;
};

static void
layer_surface_configure(void *data, struct zwlr_layer_surface_v1 *surface, uint32_t serial, uint32_t w, uint32_t h)
{
	zwlr_layer_surface_v1_ack_configure(surface, serial);
}

static void
layer_surface_closed(void *data, struct zwlr_layer_surface_v1 *surface)
{
}

static void
handle_global(void *data, struct wl_registry *registry, uint32_t name, const char *interface, uint32_t version)
{
	struct wob *app = (struct wob *) data;

	if (strcmp(interface, wl_shm_interface.name) == 0) {
		app->wl_shm = wl_registry_bind(registry, name, &wl_shm_interface, 1);
	}
	else if (strcmp(interface, wl_compositor_interface.name) == 0) {
		app->wl_compositor = wl_registry_bind(registry, name, &wl_compositor_interface, 1);
	}
	else if (strcmp(interface, xdg_wm_base_interface.name) == 0) {
		app->xdg_wm_base = wl_registry_bind(registry, name, &xdg_wm_base_interface, 1);
	}
	else if (strcmp(interface, "wl_output") == 0) {
		if (!app->wl_output) {
			app->wl_output = wl_registry_bind(registry, name, &wl_output_interface, 1);
		}
	}
	else if (strcmp(interface, zwlr_layer_shell_v1_interface.name) == 0) {
		app->zwlr_layer_shell = wl_registry_bind(registry, name, &zwlr_layer_shell_v1_interface, 1);
	}
}

static void
handle_global_remove(void *data, struct wl_registry *registry, uint32_t name)
{
}

static uint32_t *
wob_create_argb_buffer(struct wob *app)
{
	int shmid = -1;
	char shm_name[8] = {0};
	for (uint8_t i = 0; i < UINT8_MAX; ++i) {
		sprintf(shm_name, "wob-%d", i);
		shmid = shm_open(shm_name, O_RDWR | O_CREAT | O_EXCL, 0600);
		if (shmid > 0) {
			break;
		}
	}

	if (shmid < 0) {
		fprintf(stderr, "shm_open failed\n");
		exit(EXIT_FAILURE);
	}

	shm_unlink(shm_name);
	if (ftruncate(shmid, SIZE) < 0) {
		close(shmid);
		fprintf(stderr, "ftruncate failed\n");
		exit(EXIT_FAILURE);
	}

	void *shm_data = mmap(NULL, SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shmid, 0);
	if (shm_data == MAP_FAILED) {
		fprintf(stderr, "mmap failed\n");
		exit(EXIT_FAILURE);
	}

	app->shmid = shmid;

	return (uint32_t *) shm_data;
}

static void
wob_create_surface(struct wob *app)
{
	const static struct wl_registry_listener wl_registry_listener = {
		.global = handle_global,
		.global_remove = handle_global_remove,
	};

	const static struct zwlr_layer_surface_v1_listener zwlr_layer_surface_listener = {
		.configure = layer_surface_configure,
		.closed = layer_surface_closed,
	};

	app->wl_registry = wl_display_get_registry(app->wl_display);
	if (app->wl_registry == NULL) {
		fprintf(stderr, "wl_display_get_registry failed\n");
		exit(EXIT_FAILURE);
	}

	wl_registry_add_listener(app->wl_registry, &wl_registry_listener, app);

	if (wl_display_roundtrip(app->wl_display) == -1) {
		fprintf(stderr, "wl_display_roundtrip failed\n");
		exit(EXIT_FAILURE);
	}

	struct wl_shm_pool *pool = wl_shm_create_pool(app->wl_shm, app->shmid, SIZE);
	if (pool == NULL) {
		fprintf(stderr, "wl_shm_create_pool failed\n");
		exit(EXIT_FAILURE);
	}

	app->wl_buffer = wl_shm_pool_create_buffer(pool, 0, WIDTH, HEIGHT, STRIDE, WL_SHM_FORMAT_ARGB8888);
	wl_shm_pool_destroy(pool);
	if (app->wl_buffer == NULL) {
		fprintf(stderr, "wl_shm_pool_create_buffer failed\n");
		exit(EXIT_FAILURE);
	}

	app->wl_surface = wl_compositor_create_surface(app->wl_compositor);
	if (app->wl_surface == NULL) {
		fprintf(stderr, "wl_compositor_create_surface failed\n");
		exit(EXIT_FAILURE);
	}
	app->zwlr_layer_surface = zwlr_layer_shell_v1_get_layer_surface(app->zwlr_layer_shell, app->wl_surface, app->wl_output, ZWLR_LAYER_SHELL_V1_LAYER_OVERLAY, "wob");
	if (app->zwlr_layer_surface == NULL) {
		fprintf(stderr, "wl_compositor_create_surface failed\n");
		exit(EXIT_FAILURE);
	}

	zwlr_layer_surface_v1_set_size(app->zwlr_layer_surface, WIDTH, HEIGHT);
	zwlr_layer_surface_v1_set_anchor(app->zwlr_layer_surface, 0);
	zwlr_layer_surface_v1_add_listener(app->zwlr_layer_surface, &zwlr_layer_surface_listener, app->zwlr_layer_surface);

	wl_surface_commit(app->wl_surface);
	if (wl_display_roundtrip(app->wl_display) == -1) {
		fprintf(stderr, "wl_display_roundtrip failed\n");
		exit(EXIT_FAILURE);
	}
}

static void
wob_flush(struct wob *app)
{
	wl_surface_attach(app->wl_surface, app->wl_buffer, 0, 0);
	wl_surface_damage(app->wl_surface, 0, 0, WIDTH, HEIGHT);
	wl_surface_commit(app->wl_surface);
	if (wl_display_dispatch(app->wl_display) == -1) {
		fprintf(stderr, "wl_display_dispatch failed\n");
		exit(EXIT_FAILURE);
	}
}

static void
wob_destroy_surface(struct wob *app)
{
	if (app->wl_registry == NULL) {
		return;
	}

	zwlr_layer_surface_v1_destroy(app->zwlr_layer_surface);
	zwlr_layer_shell_v1_destroy(app->zwlr_layer_shell);
	wl_surface_destroy(app->wl_surface);
	wl_registry_destroy(app->wl_registry);
	xdg_wm_base_destroy(app->xdg_wm_base);
	wl_buffer_destroy(app->wl_buffer);
	wl_compositor_destroy(app->wl_compositor);
	wl_shm_destroy(app->wl_shm);
	wl_output_destroy(app->wl_output);

	app->wl_buffer = NULL;
	app->wl_compositor = NULL;
	app->wl_output = NULL;
	app->wl_registry = NULL;
	app->wl_shm = NULL;
	app->wl_surface = NULL;
	app->xdg_wm_base = NULL;
	app->zwlr_layer_shell = NULL;
	app->zwlr_layer_surface = NULL;

	if (wl_display_roundtrip(app->wl_display) == -1) {
		fprintf(stderr, "wl_display_roundtrip failed\n");
		exit(EXIT_FAILURE);
	}
}

static void
wob_destroy(struct wob *app)
{
	wob_destroy_surface(app);
	wl_display_disconnect(app->wl_display);
}

static bool
wob_parse_input(char *input_buffer, uint8_t *percentage)
{
	char *strtoul_end, *newline_position;
	unsigned long parsed_percentage;

	newline_position = strchr(input_buffer, '\n');
	if (newline_position == NULL) {
		return false;
	}

	if (newline_position == input_buffer) {
		return false;
	}

	parsed_percentage = strtoul(input_buffer, &strtoul_end, 10);
	if (strtoul_end != newline_position) {
		return false;
	}

	if (parsed_percentage > 100) {
		return false;
	}

	*percentage = parsed_percentage;

	return true;
}

int
main(int argc, char **argv)
{
	const char *usage = 
		"Usage: wob [options]\n"
		"\n"
		"  -h      Show help message and quit.\n"
		"  -v      Show the version number and quit.\n"
		"  -t <ms> Hide wob after <ms> milliseconds, defaults to 1000.\n"
		"\n"
	;

	struct wob app = {0};

	app.wl_display = wl_display_connect(NULL);
	assert(app.wl_display);
	if (app.wl_display == NULL) {
		fprintf(stderr, "wl_display_connect failed\n");
		return EXIT_FAILURE;
	}

	// Parse arguments
	int c;
	int timeout_msec = 1000;
	while ((c = getopt(argc, argv, "t:vh")) != -1) {
		switch (c){
			case 't':
				timeout_msec = atoi(optarg);
				if (timeout_msec < 0){
					fprintf(stderr, "Timeout must be a positive value.");
					return EXIT_FAILURE;
				}
				break;
			case 'v':
				fprintf(stdout, "wob version: " WOB_VERSION "\n");
				return EXIT_SUCCESS;
			case 'h':
				fprintf(stdout, "%s", usage);
				return EXIT_SUCCESS;
			default:
				fprintf(stderr, "%s", usage);
				exit(EXIT_FAILURE);
		}
	}

	uint32_t *argb = wob_create_argb_buffer(&app), i, k;
	assert(argb);
	assert(app.shmid);

	// start with all black
	for (i = 0; i < WIDTH * HEIGHT; ++i) {
		argb[i] = BLACK;
	}

	// create top and bottom line
	i = WIDTH * BORDER_OFFSET;
	k = WIDTH * (HEIGHT - BORDER_OFFSET - BORDER_SIZE);
	for (int line = 0; line < BORDER_SIZE; ++line) {
		i += BORDER_OFFSET;
		k += BORDER_OFFSET;
		for (int pixel = 0; pixel < WIDTH - 2 * BORDER_OFFSET; ++pixel) {
			argb[i++] = WHITE;
			argb[k++] = WHITE;
		}
		i += BORDER_OFFSET;
		k += BORDER_OFFSET;
	}

	// create left and right horizontal line
	i = WIDTH * (BORDER_OFFSET + BORDER_SIZE);
	k = WIDTH * (BORDER_OFFSET + BORDER_SIZE);
	for (int line = 0; line < HEIGHT - 2 * (BORDER_SIZE + BORDER_OFFSET); ++line) {
		i += BORDER_OFFSET;
		k += WIDTH - BORDER_OFFSET - BORDER_SIZE;
		for (int pixel = 0; pixel < BORDER_SIZE; ++pixel) {
			argb[i++] = WHITE;
			argb[k++] = WHITE;
		}
		i += WIDTH - BORDER_OFFSET - BORDER_SIZE;
		k += BORDER_OFFSET;
	}

	struct pollfd fds[2];
	fds[0] = (struct pollfd) {
		.fd = wl_display_get_fd(app.wl_display),
		.events = POLLIN,
	};
	fds[1] = (struct pollfd) {
		.fd = STDIN_FILENO,
		.events = POLLIN,
	};

	bool hidden = true;
	for (;;) {
		uint8_t percentage = 0;
		char input_buffer[6] = {0};
		char *fgets_rv;

		switch (poll(fds, 2, hidden ? -1 : timeout_msec)) {
			case -1:
				perror("poll");
				wob_destroy(&app);
				return EXIT_FAILURE;
			case 0:
				if (!hidden) {
					wob_destroy_surface(&app);
				}

				hidden = true;
				break;
			default:
				if (fds[0].revents & POLLIN) {
					if (wl_display_dispatch(app.wl_display) == -1) {
						wob_destroy(&app);
						return EXIT_FAILURE;
					}
				}

				if (!(fds[1].revents & POLLIN)) {
					break;
				}

				fgets_rv = fgets(input_buffer, 6, stdin);
				if (feof(stdin)) {
					wob_destroy(&app);
					return EXIT_SUCCESS;
				}

				if (fgets_rv == NULL || !wob_parse_input(input_buffer, &percentage)) {
					fprintf(stderr, "Received invalid input\n");
					wob_destroy(&app);
					return EXIT_FAILURE;
				}

				if (hidden) {
					wob_create_surface(&app);

					assert(app.wl_buffer);
					assert(app.wl_compositor);
					assert(app.wl_output);
					assert(app.wl_registry);
					assert(app.wl_shm);
					assert(app.wl_surface);
					assert(app.xdg_wm_base);
					assert(app.zwlr_layer_shell);
					assert(app.zwlr_layer_surface);
				}

				// clear percentage
				i = WIDTH * (2 * BORDER_OFFSET + BORDER_SIZE);
				for (int line = 0; line < HEIGHT - 2 * BORDER_SIZE - 4 * BORDER_OFFSET; ++line) {
					i += 2 * BORDER_OFFSET + BORDER_SIZE;
					for (int pixel = 0; pixel < (WIDTH - 2 * BORDER_SIZE - 4 * BORDER_OFFSET); ++pixel) {
						argb[i++] = BLACK;
					}
					i += 2 * BORDER_OFFSET + BORDER_SIZE;
				}

				// render percentage bar
				uint32_t bar_length = ((WIDTH - 2 * BORDER_SIZE - 4 * BORDER_OFFSET) * percentage) / 100;
				i = WIDTH * (2 * BORDER_OFFSET + BORDER_SIZE);
				for (int line = 0; line < HEIGHT - 2 * BORDER_SIZE - 4 * BORDER_OFFSET; ++line) {
					i += 2 * BORDER_OFFSET + BORDER_SIZE;
					for (int pixel = 0; pixel < bar_length; ++pixel) {
						argb[i++] = WHITE;
					}
					i += 2 * BORDER_OFFSET + BORDER_SIZE + ((WIDTH - 2 * BORDER_SIZE - 4 * BORDER_OFFSET) - bar_length);
				}

				wob_flush(&app);
				hidden = false;

				break;
		}
	}
}

#define DEFAULT_WIDTH 400
#define DEFAULT_HEIGHT 50
#define DEFAULT_BORDER_OFFSET 4
#define DEFAULT_BORDER_SIZE 4
#define DEFAULT_BAR_PADDING 4
#define DEFAULT_ANCHOR 0
#define DEFAULT_MARGIN 0
#define MIN_PERCENTAGE_BAR_WIDTH 1
#define MIN_PERCENTAGE_BAR_HEIGHT 1

#define BLACK 0xFF000000
#define WHITE 0xFFFFFFFF

// sizeof already includes NULL byte
#define INPUT_BUFFER_LENGTH (3 * sizeof(unsigned long) + sizeof(" #FF000000 #FFFFFFFF #FFFFFFFF\n"))

#define MIN(a, b) (((a) < (b)) ? (a) : (b))

#define _POSIX_C_SOURCE 200809L
#include <assert.h>
#include <errno.h>
#include <fcntl.h> // shm
#include <limits.h>
#include <poll.h>
#include <stdbool.h>  // true, false
#include <stdio.h>    // NULL
#include <stdlib.h>   // EXIT_FAILURE
#include <string.h>   // strcmp
#include <sys/mman.h> // shm
#include <unistd.h>   // shm, ftruncate

#ifdef WOB_USE_SECCOMP
#include <linux/audit.h>
#include <linux/filter.h>
#include <linux/seccomp.h>
#include <linux/signal.h>
#include <seccomp.h>
#include <sys/ptrace.h>
#endif

#include "wlr-layer-shell-unstable-v1-client-protocol.h"
#include "xdg-shell-client-protocol.h"

typedef uint32_t argb_color;

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

struct wob_output {
	struct wl_list link;
	struct wl_output *wl_output;
	struct wl_surface *wl_surface;
	struct zwlr_layer_surface_v1 *zwlr_layer_surface;
};

struct wob {
	int shmid;
	struct wl_buffer *wl_buffer;
	struct wl_compositor *wl_compositor;
	struct wl_display *wl_display;
	struct wl_list wob_outputs;
	struct wl_registry *wl_registry;
	struct wl_shm *wl_shm;
	struct wob_geom *wob_geom;
	struct xdg_wm_base *xdg_wm_base;
	struct zwlr_layer_shell_v1 *zwlr_layer_shell;
};

void
layer_surface_configure(void *data, struct zwlr_layer_surface_v1 *surface, uint32_t serial, uint32_t w, uint32_t h)
{
	zwlr_layer_surface_v1_ack_configure(surface, serial);
}

void
layer_surface_closed(void *data, struct zwlr_layer_surface_v1 *surface)
{
}

void
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
		struct wob_output *output = calloc(1, sizeof(struct wob_output));
		output->wl_output = wl_registry_bind(registry, name, &wl_output_interface, 1);
		wl_list_insert(&app->wob_outputs, &output->link);
	}
	else if (strcmp(interface, zwlr_layer_shell_v1_interface.name) == 0) {
		app->zwlr_layer_shell = wl_registry_bind(registry, name, &zwlr_layer_shell_v1_interface, 1);
	}
}

void
handle_global_remove(void *data, struct wl_registry *registry, uint32_t name)
{
}

argb_color *
wob_create_argb_buffer(struct wob *app)
{
	int shmid = -1;
	char shm_name[NAME_MAX];
	for (unsigned char i = 0; i < UCHAR_MAX; ++i) {
		if (snprintf(shm_name, NAME_MAX, "/wob-%hhu", i) >= NAME_MAX) {
			break;
		}
		shmid = shm_open(shm_name, O_RDWR | O_CREAT | O_EXCL, 0600);
		if (shmid > 0 || errno != EEXIST) {
			break;
		}
	}

	if (shmid < 0) {
		perror("shm_open");
		exit(EXIT_FAILURE);
	}

	if (shm_unlink(shm_name) != 0) {
		perror("shm_unlink");
		exit(EXIT_FAILURE);
	}

	if (ftruncate(shmid, app->wob_geom->size) != 0) {
		perror("ftruncate");
		exit(EXIT_FAILURE);
	}

	void *shm_data = mmap(NULL, app->wob_geom->size, PROT_READ | PROT_WRITE, MAP_SHARED, shmid, 0);
	if (shm_data == MAP_FAILED) {
		perror("mmap");
		exit(EXIT_FAILURE);
	}

	app->shmid = shmid;

	return (argb_color *) shm_data;
}

void
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

	wl_list_init(&app->wob_outputs);

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

	struct wob_output *output, *tmp;
	wl_list_for_each_safe (output, tmp, &app->wob_outputs, link) {
		output->wl_surface = wl_compositor_create_surface(app->wl_compositor);
		if (output->wl_surface == NULL) {
			fprintf(stderr, "wl_compositor_create_surface failed\n");
			exit(EXIT_FAILURE);
		}

		output->zwlr_layer_surface = zwlr_layer_shell_v1_get_layer_surface(app->zwlr_layer_shell, output->wl_surface, output->wl_output, ZWLR_LAYER_SHELL_V1_LAYER_OVERLAY, "wob");
		if (output->zwlr_layer_surface == NULL) {
			fprintf(stderr, "wl_compositor_create_surface failed\n");
			exit(EXIT_FAILURE);
		}
		zwlr_layer_surface_v1_set_size(output->zwlr_layer_surface, app->wob_geom->width, app->wob_geom->height);
		zwlr_layer_surface_v1_set_anchor(output->zwlr_layer_surface, app->wob_geom->anchor);
		zwlr_layer_surface_v1_set_margin(output->zwlr_layer_surface, app->wob_geom->margin, app->wob_geom->margin, app->wob_geom->margin, app->wob_geom->margin);
		zwlr_layer_surface_v1_add_listener(output->zwlr_layer_surface, &zwlr_layer_surface_listener, output);
		wl_surface_commit(output->wl_surface);
	}

	if (wl_display_roundtrip(app->wl_display) == -1) {
		fprintf(stderr, "wl_display_roundtrip failed\n");
		exit(EXIT_FAILURE);
	}
}

void
wob_flush(struct wob *app)
{
	struct wob_output *output, *tmp;
	wl_list_for_each_safe (output, tmp, &app->wob_outputs, link) {
		wl_surface_attach(output->wl_surface, app->wl_buffer, 0, 0);
		wl_surface_damage(output->wl_surface, 0, 0, app->wob_geom->width, app->wob_geom->height);
		wl_surface_commit(output->wl_surface);
	}

	if (wl_display_dispatch(app->wl_display) == -1) {
		fprintf(stderr, "wl_display_dispatch failed\n");
		exit(EXIT_FAILURE);
	}
}

void
wob_destroy_surface(struct wob *app)
{
	if (app->wl_registry == NULL) {
		return;
	}

	zwlr_layer_shell_v1_destroy(app->zwlr_layer_shell);
	wl_registry_destroy(app->wl_registry);
	xdg_wm_base_destroy(app->xdg_wm_base);
	wl_buffer_destroy(app->wl_buffer);
	wl_compositor_destroy(app->wl_compositor);
	wl_shm_destroy(app->wl_shm);

	app->zwlr_layer_shell = NULL;
	app->wl_registry = NULL;
	app->xdg_wm_base = NULL;
	app->wl_buffer = NULL;
	app->wl_compositor = NULL;
	app->wl_shm = NULL;

	struct wob_output *output, *tmp;
	wl_list_for_each_safe (output, tmp, &app->wob_outputs, link) {
		zwlr_layer_surface_v1_destroy(output->zwlr_layer_surface);
		wl_output_destroy(output->wl_output);
		wl_surface_destroy(output->wl_surface);

		free(output);
	}

	if (wl_display_roundtrip(app->wl_display) == -1) {
		fprintf(stderr, "wl_display_roundtrip failed\n");
		exit(EXIT_FAILURE);
	}
}

void
wob_destroy(struct wob *app)
{
	wob_destroy_surface(app);
	wl_display_disconnect(app->wl_display);
}

/*
Input format:
percentage bgColor borderColor barColor
25 #FF000000 #FFFFFFFF #FFFFFFFF
*/
bool
wob_parse_input(const char *input_buffer, unsigned long *percentage, argb_color *background_color, argb_color *border_color, argb_color *bar_color)
{
	char *input_ptr, *newline_position;

	newline_position = strchr(input_buffer, '\n');
	if (newline_position == NULL) {
		return false;
	}

	if (newline_position == input_buffer) {
		return false;
	}

	*percentage = strtoul(input_buffer, &input_ptr, 10);
	if (input_ptr == newline_position) {
		return true;
	}

	if (input_ptr + 10 > newline_position || input_ptr[0] != ' ' || input_ptr[1] != '#') {
		return false;
	}
	input_ptr += 2;
	*background_color = strtoul(input_ptr, &input_ptr, 16);

	if (input_ptr + 10 > newline_position || input_ptr[0] != ' ' || input_ptr[1] != '#') {
		return false;
	}
	input_ptr += 2;
	*border_color = strtoul(input_ptr, &input_ptr, 16);

	if (input_ptr + 10 > newline_position || input_ptr[0] != ' ' || input_ptr[1] != '#') {
		return false;
	}
	input_ptr += 2;
	*bar_color = strtoul(input_ptr, &input_ptr, 16);

	if (*input_ptr != '\n') {
		return false;
	}

	return true;
}

void
wob_draw_background(const struct wob_geom *geom, argb_color *argb, argb_color color)
{
	for (size_t i = 0; i < geom->width * geom->height; ++i) {
		argb[i] = color;
	}
}

void
wob_draw_border(const struct wob_geom *geom, argb_color *argb, argb_color color)
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
wob_draw_percentage(const struct wob_geom *geom, argb_color *argb, argb_color bar_color, argb_color background_color, unsigned long percentage, unsigned long maximum)
{
	size_t offset_border_padding = geom->border_offset + geom->border_size + geom->bar_padding;
	size_t bar_width = geom->width - 2 * offset_border_padding;
	size_t bar_height = geom->height - 2 * offset_border_padding;
	size_t bar_colored_width = (bar_width * percentage) / maximum;

	// draw 1px horizontal line
	argb_color *start, *end, *pixel;
	start = &argb[offset_border_padding * (geom->width + 1)];
	end = start + bar_colored_width;
	for (pixel = start; pixel < end; ++pixel) {
		*pixel = bar_color;
	}
	for (end = start + bar_width; pixel < end; ++pixel) {
		*pixel = background_color;
	}

	// copy it to make full percentage bar
	argb_color *source = &argb[offset_border_padding * geom->width];
	argb_color *destination = source + geom->width;
	end = &argb[geom->width * (bar_height + offset_border_padding)];
	while (destination != end) {
		memcpy(destination, source, MIN(destination - source, end - destination) * sizeof(argb_color));
		destination += MIN(destination - source, end - destination);
	}
}

void
wob_pledge(void)
{
#ifdef WOB_USE_SECCOMP
	const int scmp_sc[] = {
		SCMP_SYS(close),
		SCMP_SYS(exit),
		SCMP_SYS(exit_group),
		SCMP_SYS(fcntl),
		SCMP_SYS(fstat),
		SCMP_SYS(poll),
		SCMP_SYS(read),
		SCMP_SYS(readv),
		SCMP_SYS(recvmsg),
		SCMP_SYS(restart_syscall),
		SCMP_SYS(sendmsg),
		SCMP_SYS(write),
		SCMP_SYS(writev),
	};

	int ret;
	scmp_filter_ctx scmp_ctx = seccomp_init(SCMP_ACT_KILL);
	if (scmp_ctx == NULL) {
		fprintf(stderr, "seccomp_init(SCMP_ACT_KILL) failed\n");
		exit(EXIT_FAILURE);
	}

	for (size_t i = 0; i < sizeof(scmp_sc) / sizeof(int); ++i) {
		if ((ret = seccomp_rule_add(scmp_ctx, SCMP_ACT_ALLOW, scmp_sc[i], 0)) < 0) {
			fprintf(stderr, "seccomp_rule_add(scmp_ctxm, SCMP_ACT_ALLOW, %d) failed with return value %d\n", scmp_sc[i], ret);
			seccomp_release(scmp_ctx);
			exit(EXIT_FAILURE);
		}
	}

	if ((ret = seccomp_load(scmp_ctx)) < 0) {
		fprintf(stderr, "seccomp_load(scmp_ctx) failed with return value %d\n", ret);
		seccomp_release(scmp_ctx);
		exit(EXIT_FAILURE);
	}

	seccomp_release(scmp_ctx);
#endif
}

#ifndef WOB_TEST
int
main(int argc, char **argv)
{
	const char *usage =
		"Usage: wob [options]\n"
		"\n"
		"  -h      Show help message and quit.\n"
		"  -v      Show the version number and quit.\n"
		"  -t <ms> Hide wob after <ms> milliseconds, defaults to 1000.\n"
		"  -m <%>  Define the maximum percentage, defaults to 100. \n"
		"  -W <px> Define display width in pixels, defaults to 400. \n"
		"  -H <px> Define display height in pixels, defaults to 50. \n"
		"  -o <px> Define border offset in pixels, defaults to 4. \n"
		"  -b <px> Define border size in pixels, defaults to 4. \n"
		"  -p <px> Define bar padding in pixels, defaults to 4. \n"
		"  -a <s>  Define anchor point; one of 'top', 'left', 'right', 'bottom', 'center' (default). \n"
		"          May be specified multiple times. \n"
		"  -M <px> Define anchor margin in pixels, defaults to 0. \n"
		"\n";

	struct wob app = {0};

	app.wl_display = wl_display_connect(NULL);
	assert(app.wl_display);
	if (app.wl_display == NULL) {
		fprintf(stderr, "wl_display_connect failed\n");
		return EXIT_FAILURE;
	}

	// Parse arguments
	int c;
	unsigned long maximum = 100;
	unsigned long timeout_msec = 1000;
	struct wob_geom geom = {
		.width = DEFAULT_WIDTH,
		.height = DEFAULT_HEIGHT,
		.border_offset = DEFAULT_BORDER_OFFSET,
		.border_size = DEFAULT_BORDER_SIZE,
		.bar_padding = DEFAULT_BAR_PADDING,
		.anchor = DEFAULT_ANCHOR,
		.margin = DEFAULT_MARGIN,
	};
	char *strtoul_end;

	while ((c = getopt(argc, argv, "t:m:W:H:o:b:p:a:M:vh")) != -1) {
		switch (c) {
			case 't':
				timeout_msec = strtoul(optarg, &strtoul_end, 10);
				if (*strtoul_end != '\0' || errno == ERANGE || timeout_msec == 0) {
					fprintf(stderr, "Timeout must be a value between 1 and %lu.\n", ULONG_MAX);
					return EXIT_FAILURE;
				}
				break;
			case 'm':
				maximum = strtoul(optarg, &strtoul_end, 10);
				if (*strtoul_end != '\0' || errno == ERANGE || maximum == 0) {
					fprintf(stderr, "Maximum must be a value between 1 and %lu.\n", ULONG_MAX);
					return EXIT_FAILURE;
				}
				break;
			case 'W':
				geom.width = strtoul(optarg, &strtoul_end, 10);
				if (*strtoul_end != '\0' || errno == ERANGE) {
					fprintf(stderr, "Width must be a positive value.");
					return EXIT_FAILURE;
				}
				break;
			case 'H':
				geom.height = strtoul(optarg, &strtoul_end, 10);
				if (*strtoul_end != '\0' || errno == ERANGE) {
					fprintf(stderr, "Height must be a positive value.");
					return EXIT_FAILURE;
				}
				break;
			case 'o':
				geom.border_offset = strtoul(optarg, &strtoul_end, 10);
				if (*strtoul_end != '\0' || errno == ERANGE) {
					fprintf(stderr, "Border offset must be a positive value.");
					return EXIT_FAILURE;
				}
				break;
			case 'b':
				geom.border_size = strtoul(optarg, &strtoul_end, 10);
				if (*strtoul_end != '\0' || errno == ERANGE) {
					fprintf(stderr, "Border size must be a positive value.");
					return EXIT_FAILURE;
				}
				break;
			case 'p':
				geom.bar_padding = strtoul(optarg, &strtoul_end, 10);
				if (*strtoul_end != '\0' || errno == ERANGE) {
					fprintf(stderr, "Bar padding must be a positive value.");
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
					fprintf(stderr, "Anchor must be one of 'top', 'bottom', 'left', 'right', 'center'.");
					return EXIT_FAILURE;
				}
				break;
			case 'M':
				geom.margin = strtoul(optarg, &strtoul_end, 10);
				if (*strtoul_end != '\0' || errno == ERANGE) {
					fprintf(stderr, "Anchor margin must be a positive value.");
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
				return EXIT_FAILURE;
		}
	}

	if (geom.width < MIN_PERCENTAGE_BAR_WIDTH + 2 * (geom.border_offset + geom.border_size + geom.bar_padding)) {
		fprintf(stderr, "Invalid geometry: width is too small for given parameters\n");
		return EXIT_FAILURE;
	}

	if (geom.height < MIN_PERCENTAGE_BAR_HEIGHT + 2 * (geom.border_offset + geom.border_size + geom.bar_padding)) {
		fprintf(stderr, "Invalid geometry: height is too small for given parameters\n");
		return EXIT_FAILURE;
	}

	geom.stride = geom.width * 4;
	geom.size = geom.stride * geom.height;
	app.wob_geom = &geom;

	argb_color *argb = wob_create_argb_buffer(&app);
	assert(argb);
	assert(app.shmid);

	wob_pledge();

	argb_color background_color = BLACK;
	argb_color bar_color = WHITE;
	argb_color border_color = WHITE;

	// Draw these at least once
	wob_draw_background(app.wob_geom, argb, background_color);
	wob_draw_border(app.wob_geom, argb, border_color);

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

	argb_color old_background_color, old_border_color;
	bool hidden = true;
	for (;;) {
		unsigned long percentage = 0;
		char input_buffer[INPUT_BUFFER_LENGTH] = {0};
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

				fgets_rv = fgets(input_buffer, INPUT_BUFFER_LENGTH, stdin);
				if (feof(stdin)) {
					wob_destroy(&app);
					return EXIT_SUCCESS;
				}

				old_background_color = background_color;
				old_border_color = border_color;
				if (fgets_rv == NULL || !wob_parse_input(input_buffer, &percentage, &background_color, &border_color, &bar_color) || percentage > maximum) {
					fprintf(stderr, "Received invalid input\n");
					wob_destroy(&app);
					return EXIT_FAILURE;
				}

				if (hidden) {
					wob_create_surface(&app);

					assert(app.wl_buffer);
					assert(app.wl_compositor);
					assert(app.wl_registry);
					assert(app.wl_shm);
					assert(app.xdg_wm_base);
					assert(app.zwlr_layer_shell);
					assert(&app.wob_outputs);
				}

				if (old_background_color != background_color || old_border_color != border_color) {
					wob_draw_background(app.wob_geom, argb, background_color);
					wob_draw_border(app.wob_geom, argb, border_color);
				}
				wob_draw_percentage(app.wob_geom, argb, bar_color, background_color, percentage, maximum);

				wob_flush(&app);
				hidden = false;

				break;
		}
	}
}
#endif

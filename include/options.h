#ifndef _WOB_OPTIONS_H
#define _WOB_OPTIONS_H

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
#define WOB_DEFAULT_PLEDGE true

#include <stdbool.h>
#include <stdint.h>
#include <wayland-util.h>

struct wob_output_name {
	char *name;
	struct wl_list link;
};

struct wob_options {
	uint32_t border_color;
	uint32_t background_color;
	uint32_t bar_color;
	unsigned long timeout_msec;
	unsigned long maximum;
	unsigned long bar_width;
	unsigned long bar_height;
	unsigned long bar_border;
	unsigned long bar_padding;
	unsigned long bar_margin;
	unsigned long border_offset;
	unsigned long border_size;
	int bar_anchor;
	struct wl_list outputs;
	bool pledge;
};

bool wob_getopt(const int argc, char **argv, struct wob_options *options);

void wob_options_destroy(struct wob_options *options);

#endif
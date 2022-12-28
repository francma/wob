#ifndef _WOB_BUFFER_H
#define _WOB_BUFFER_H

#include <stddef.h>
#include <stdint.h>

#include "config.h"

struct wob_image {
	int shmid;
	size_t width;
	size_t height;
	size_t size_in_bytes;
	size_t stride;
	void *_data;
};

struct wob_image *wob_image_create_argb8888(size_t width, size_t height);

void wob_image_destroy(struct wob_image *image);

void wob_image_draw(struct wob_image *image, struct wob_colors colors, struct wob_dimensions dimensions, unsigned long percentage, unsigned long maximum);

#endif

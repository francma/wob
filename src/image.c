#define WOB_FILE "buffer.c"

#define _POSIX_C_SOURCE 200112L

#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <pixman.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

#include "image.h"
#include "log.h"

struct wob_image *
wob_image_create_argb8888(size_t width, size_t height)
{
	int shmid = -1;
	char shm_name[NAME_MAX];
	for (int i = 0; i < UCHAR_MAX; ++i) {
		if (snprintf(shm_name, NAME_MAX, "/wob-%d", i) >= NAME_MAX) {
			break;
		}
		shmid = shm_open(shm_name, O_RDWR | O_CREAT | O_EXCL, 0600);
		if (shmid > 0 || errno != EEXIST) {
			break;
		}
	}

	if (shmid < 0) {
		wob_log_error("shm_open() failed: %s", strerror(errno));
		return NULL;
	}

	if (shm_unlink(shm_name) != 0) {
		wob_log_error("shm_unlink() failed: %s", strerror(errno));
		return NULL;
	}

	size_t size = width * height * 8;
	if (ftruncate(shmid, size) != 0) {
		wob_log_error("ftruncate() failed: %s", strerror(errno));
		return NULL;
	}

	void *buffer = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, shmid, 0);
	if (buffer == MAP_FAILED) {
		wob_log_error("mmap() failed: %s", strerror(errno));
		return NULL;
	}

	pixman_image_t *pixman = pixman_image_create_bits_no_clear(PIXMAN_a8r8g8b8, width, height, buffer, width * 4);

	struct wob_image *image = malloc(sizeof(struct wob_image));

	image->shmid = shmid;
	image->width = width;
	image->height = height;
	image->size_in_bytes = width * height * 4;
	image->stride = width * 4;
	image->_data = pixman;

	return image;
}

void
wob_image_destroy(struct wob_image *image)
{
	pixman_image_t *pixman = image->_data;

	pixman_image_unref(pixman);
	free(image);
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
wob_image_draw(struct wob_image *image, struct wob_colors colors, struct wob_dimensions dimensions, unsigned long percentage, unsigned long maximum)
{
	pixman_image_t *pixman = image->_data;

	pixman_color_t bar_color = wob_color_to_pixman(wob_color_premultiply_alpha(colors.value));
	pixman_color_t background_color = wob_color_to_pixman(wob_color_premultiply_alpha(colors.background));
	pixman_color_t border_color = wob_color_to_pixman(wob_color_premultiply_alpha(colors.border));

	size_t offset_border_padding = dimensions.border_offset + dimensions.border_size + dimensions.bar_padding;
	size_t bar_width = dimensions.width - 2 * offset_border_padding;
	size_t bar_height = dimensions.height - 2 * offset_border_padding;

	uint32_t offset = 0;
	pixman_image_fill_rectangles(PIXMAN_OP_SRC, pixman, &background_color, 1, &(pixman_rectangle16_t){0, 0, dimensions.width, dimensions.height});
	offset += dimensions.border_offset;
	pixman_image_fill_rectangles(PIXMAN_OP_SRC, pixman, &border_color, 1, &(pixman_rectangle16_t){offset, offset, dimensions.width - 2 * offset, dimensions.height - 2 * offset});
	offset += dimensions.border_size;
	pixman_image_fill_rectangles(PIXMAN_OP_SRC, pixman, &background_color, 1, &(pixman_rectangle16_t){offset, offset, dimensions.width - 2 * offset, dimensions.height - 2 * offset});

	offset += dimensions.bar_padding;
	uint32_t filled_width = (bar_width * percentage) / maximum;
	uint32_t filled_height = (bar_height * percentage) / maximum;
	switch (dimensions.orientation) {
		case WOB_ORIENTATION_HORIZONTAL:
			pixman_image_fill_rectangles(PIXMAN_OP_SRC, pixman, &bar_color, 1, &(pixman_rectangle16_t){offset, offset, filled_width, bar_height});
			break;
		case WOB_ORIENTATION_VERTICAL:
			pixman_image_fill_rectangles(PIXMAN_OP_SRC, pixman, &bar_color, 1, &(pixman_rectangle16_t){offset, offset + bar_height - filled_height, bar_width, filled_height});
			break;
	}
}

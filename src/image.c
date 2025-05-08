#define WOB_FILE "image.c"

#include "image.h"

void
fill_rectangle(uint32_t *pixels, size_t width, size_t height, size_t stride, uint32_t color)
{
	for (size_t y = 0; y < height; ++y) {
		for (size_t x = 0; x < width; ++x) {
			pixels[x] = color;
		}
		pixels += stride;
	}
}

void
wob_image_draw(uint32_t *image_data, struct wob_dimensions dimensions, struct wob_colors colors, double percentage)
{
	uint32_t bar_color = wob_color_to_argb(wob_color_premultiply_alpha(colors.value));
	uint32_t background_color = wob_color_to_argb(wob_color_premultiply_alpha(colors.background));
	uint32_t border_color = wob_color_to_argb(wob_color_premultiply_alpha(colors.border));

	uint32_t *data;
	uint32_t height;
	uint32_t width;
	uint32_t offset;
	uint32_t stride = dimensions.width;

	height = dimensions.height;
	width = dimensions.width;
	data = image_data;
	fill_rectangle(data, width, height, stride, background_color);

	offset = dimensions.border_offset;
	height = dimensions.height - (2 * offset);
	width = dimensions.width - (2 * offset);
	data = image_data + (offset * (dimensions.width + 1));
	fill_rectangle(data, width, height, stride, border_color);

	offset = dimensions.border_offset + dimensions.border_size;
	height = dimensions.height - (2 * offset);
	width = dimensions.width - (2 * offset);
	data = image_data + (offset * (dimensions.width + 1));
	fill_rectangle(data, width, height, stride, background_color);

	offset = dimensions.border_offset + dimensions.border_size + dimensions.bar_padding;
	size_t bar_width = dimensions.width - 2 * offset;
	size_t bar_height = dimensions.height - 2 * offset;
	switch (dimensions.orientation) {
		case WOB_ORIENTATION_HORIZONTAL:
			height = bar_height;
			width = bar_width * percentage;
			data = image_data + (offset * (dimensions.width + 1));
			fill_rectangle(data, width, height, stride, bar_color);
			break;
		case WOB_ORIENTATION_VERTICAL:
			height = bar_height * percentage;
			width = bar_width;
			data = image_data + (offset * (dimensions.width + 1)) + (bar_height - height) * dimensions.width;
			fill_rectangle(data, width, height, stride, bar_color);
			break;
	}
}

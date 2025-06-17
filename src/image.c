#define WOB_FILE "image.c"

#include <stdio.h>

#include "image.h"
#include "log.h"

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
wob_image_draw(uint32_t *image_data, struct wob_dimensions dimensions, struct wob_colors colors, double percentage, struct wob_font *font, unsigned long font_size)
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

	if (font == NULL) {
		return;
	}

	size_t font_padding = font_size / 4;

	char percentage_buff[64] = {0};
	snprintf(percentage_buff, 64, "%d", (int) (percentage * 100));
	struct wob_font_text_dimensions text_dimensions = wob_font_render_text_dimensions(font, percentage_buff, font_size);
	wob_log_debug("declared font height %d, rendered text width: %d x %d\n", font_size, text_dimensions.w, text_dimensions.h);

	const uint32_t width_needed_for_text = text_dimensions.w + 2 * font_padding;
	struct wob_color font_color;

	// data is positing to the beginning of TOP LEFT corner of rendered block
	switch (dimensions.orientation) {
		case WOB_ORIENTATION_HORIZONTAL:
			// get to the X position first
			if (width_needed_for_text < width) {
				data += width - text_dimensions.w - font_padding;
				font_color = colors.background;
			}
			else if (width_needed_for_text < bar_width) {
				data += width + font_padding;
				font_color = colors.value;
			}
			else {
				wob_log_warn("bar text is too big for the bar to be rendered, skipping!");
				return;
			}

			// get to the Y position
			data += stride * ((bar_height - text_dimensions.h) / 2);
			break;
		case WOB_ORIENTATION_VERTICAL:
			// get to the Y position first
			if (width_needed_for_text < height) {
				data += stride * font_padding;
				font_color = colors.background;
			}
			else if (width_needed_for_text < bar_height) {
				data -= stride * (text_dimensions.h + font_padding);
				font_color = colors.value;
			}
			else {
				wob_log_warn("bar text is too big for the bar to be rendered, skipping!");
				return;
			}

			// get to the X position
			data += (width - text_dimensions.w) / 2;
			break;
	}

	wob_font_render_text(font, percentage_buff, font_size, font_color, data, stride);
}

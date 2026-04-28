#include "src/color.h"
#include "src/config.h"
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wayland-util.h>
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

void draw_segments(uint32_t* image_data, unsigned long width,
                   unsigned long height, unsigned long stride,
                   double percentage, unsigned long number_of_segments,
                   struct wob_segment_bounds* segment_bounds,
                   uint32_t* segment_colours
) {
    unsigned long adjusted_width = width * percentage;

    for (int i = number_of_segments - 1; i >= 0; i--) {
        if (adjusted_width >= width * segment_bounds[i].lowerBound && segment_bounds[i].lowerBound <= segment_bounds[i].upperBound) {
            if (adjusted_width > width * segment_bounds[i].upperBound) {
                fill_rectangle(
                        image_data + (uint32_t)(width * segment_bounds[i].lowerBound),
                         (double)width * (segment_bounds[i].upperBound - segment_bounds[i].lowerBound),
                                height, stride, segment_colours[i]
                );
            }
            else {
                fill_rectangle(
                        image_data + (uint32_t)(width * segment_bounds[i].lowerBound),
                         adjusted_width - width * segment_bounds[i].lowerBound,
                                height, stride, segment_colours[i]
                );
            }
        }
    }
}

void
wob_image_draw(uint32_t *image_data, struct wob_dimensions dimensions, struct wob_colors colors, double percentage)
{
	uint32_t bar_color = wob_color_to_argb(wob_color_premultiply_alpha(colors.value));
	uint32_t background_color = wob_color_to_argb(wob_color_premultiply_alpha(colors.background));
	uint32_t border_color = wob_color_to_argb(wob_color_premultiply_alpha(colors.border));
    
    unsigned long number_of_segments = dimensions.segments.number;
    struct wob_color* unadjusted_segment_colours = colors.segmentColours;
    uint32_t *segment_colours = NULL;

    if (number_of_segments > 0 && unadjusted_segment_colours != NULL) {
        segment_colours = calloc(dimensions.segments.number, sizeof(uint32_t));
        for (unsigned long i = 0; i < dimensions.segments.number; i++) {
            segment_colours[i] = wob_color_to_argb(wob_color_premultiply_alpha(unadjusted_segment_colours[i]));
        }
    }

    struct wob_segment_bounds* segment_bounds = dimensions.segments.segmentArray;

	uint32_t *data;
	uint32_t height;
	uint32_t width;
	uint32_t offset;
	uint32_t stride = dimensions.width;

    // draw background box
	height = dimensions.height;
	width = dimensions.width;
	data = image_data;
	fill_rectangle(data, width, height, stride, background_color);

    // draw border as a box (will be drawn over to make it look like a border)
	offset = dimensions.border_offset;
	height = dimensions.height - (2 * offset);
	width = dimensions.width - (2 * offset);
	data = image_data + (offset * (dimensions.width + 1));
	fill_rectangle(data, width, height, stride, border_color);

    // fill the inner box
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

            if (number_of_segments > 0 && segment_colours != NULL) {
                draw_segments(data,
                              bar_width + 1,
                              bar_height,
                              stride,
                              percentage,
                              number_of_segments,
                              segment_bounds,
                              segment_colours
                             );

            }

			break;
		case WOB_ORIENTATION_VERTICAL:
                break;
        // 	height = bar_height * percentage;
		// 	width = bar_width;
		// 	data = image_data + (offset * (dimensions.width + 1)) + (bar_height - height) * dimensions.width;

            // for (int i = numberOfBounds - 1; i >= 0; i--) {
                // if (width >= bar_height * test.segmentArray[i].lowerBound) {
                    // if (width > bar_height * test.segmentArray[i].upperBound) {
                        // fill_rectangle(data, width, (double)bar_height * test.segmentArray[i].upperBound, stride, segment_colours[i]);
                    // }
                    // else {
                        // fill_rectangle(data, width, height, stride, segment_colours[i]);
                    // }
                // }
            // }
		// 	break;
	}

    // free();
}

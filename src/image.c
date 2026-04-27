#include "src/color.h"
#include "src/config.h"
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
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
populate_segment_array(unsigned long number, struct wob_segment_bounds arr[], struct wob_segments* segmentStruct) {
    segmentStruct->number = number;
    segmentStruct->segmentArray = calloc(number, sizeof(struct wob_segment_bounds));
    if (segmentStruct->segmentArray == NULL) { return; }

    for (unsigned long i = 0; i < number; i++) {
        segmentStruct->segmentArray[i].lowerBound = arr[i].lowerBound / 100.0f;
        segmentStruct->segmentArray[i].upperBound = arr[i].upperBound / 100.0f;
    }
}

// void
// populate_segment_colour_array(unsigned long number, struct wob_color* colourArray, )

void
wob_image_draw(uint32_t *image_data, struct wob_dimensions dimensions, struct wob_colors colors, double percentage)
{
	uint32_t bar_color = wob_color_to_argb(wob_color_premultiply_alpha(colors.value));
	uint32_t background_color = wob_color_to_argb(wob_color_premultiply_alpha(colors.background));
	uint32_t border_color = wob_color_to_argb(wob_color_premultiply_alpha(colors.border));
    
    struct wob_color colourArray[3] = {
        {1, 0.7, 0.67, 0.3},
        {1, 0.9, 0.9, 0.5},
        {1, 0.7, 0.67, 0.9}
    };
    uint32_t segment_colours[3];

    for (int i = 0; i < 3; i++) {
        segment_colours[i] = wob_color_to_argb(wob_color_premultiply_alpha(colourArray[i]));
    }

    struct wob_segments test;
    int numberOfBounds = 3;
    struct wob_segment_bounds boundsArray[3] = {{0, 30}, {40, 67}, {70, 90}};
    populate_segment_array(3, boundsArray, &test);

    for (int i = 0; i < 3; i++) {
        printf("%lf (%u), ", test.segmentArray[i].lowerBound, segment_colours[i]);
    }
    printf("\n");

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

            for (int i = numberOfBounds - 1; i >= 0; i--) {
                if (width >= bar_width * test.segmentArray[i].lowerBound) {
                    if (width > bar_width * test.segmentArray[i].upperBound) {
                        fill_rectangle(
                                data + (uint32_t)(bar_width * test.segmentArray[i].lowerBound),
                                 (double)bar_width * (test.segmentArray[i].upperBound - test.segmentArray[i].lowerBound),
                                        height, stride, segment_colours[i]
                        );
                    }
                    else {
                        fill_rectangle(
                                data + (uint32_t)(bar_width * test.segmentArray[i].lowerBound),
                                 width - bar_width * test.segmentArray[i].lowerBound,
                                        height, stride, segment_colours[i]
                        );
                    }
                }
            }

			break;
		case WOB_ORIENTATION_VERTICAL:
			height = bar_height * percentage;
			width = bar_width;
			data = image_data + (offset * (dimensions.width + 1)) + (bar_height - height) * dimensions.width;

            for (int i = numberOfBounds - 1; i >= 0; i--) {
                if (width >= bar_height * test.segmentArray[i].lowerBound) {
                    if (width > bar_height * test.segmentArray[i].upperBound) {
                        fill_rectangle(data, width, (double)bar_height * test.segmentArray[i].upperBound, stride, segment_colours[i]);
                    }
                    else {
                        fill_rectangle(data, width, height, stride, segment_colours[i]);
                    }
                }
            }
			break;
	}

    free(test.segmentArray);
}

#ifndef _WOB_BUFFER_H
#define _WOB_BUFFER_H

#include <stddef.h>
#include <stdint.h>

#include "config.h"

void wob_image_draw(uint32_t *data, struct wob_dimensions dimensions, struct wob_colors colors, double percentage);

#endif

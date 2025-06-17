#ifndef BUILD_WOB_H
#define BUILD_WOB_H

#include "config.h"
#include "font.h"

#define INPUT_BUFFER_LENGTH 255

int wob_run(struct wob_config *config, struct wob_font_manager *font_manager);

#endif

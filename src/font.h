#ifndef FONT_H
#define FONT_H

#include <stddef.h>

#include "color.h"

struct wob_font;

struct wob_rendered_text_dimensions {
	size_t w;
	size_t h;
};

struct wob_font *wob_font_create(const char *fpath);

void wob_font_destroy(struct wob_font *font);

void wob_font_render_text(struct wob_font *font, const char *text, int font_size, struct wob_color font_color, uint32_t *argb8888_buffer, size_t argb8888_buffer_stride);

struct wob_rendered_text_dimensions wob_font_render_text_dimensions(struct wob_font *font, char *text, uint8_t font_size);

#endif

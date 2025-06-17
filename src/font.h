#ifndef FONT_H
#define FONT_H

#include <stddef.h>

#include "color.h"
#include "config.h"

struct wob_font_manager;

struct wob_font;

struct wob_font_text_dimensions {
	int w;
	int h;
};

struct wob_font_manager *wob_font_manager_create();

void wob_font_manager_destroy(struct wob_font_manager *);

void wob_font_manager_load_font(struct wob_font_manager *, const char *fpath);

void wob_font_manager_load_fonts_from_config(struct wob_font_manager *, struct wob_config *);

struct wob_font *wob_font_manager_get(struct wob_font_manager *, const char *fpath);

void wob_font_render_text(struct wob_font *font, char *text, int font_size, struct wob_color font_color, uint32_t *argb8888_buffer, size_t argb8888_buffer_size);

struct wob_font_text_dimensions wob_font_render_text_dimensions(struct wob_font *font, char *text, int font_size);

#endif

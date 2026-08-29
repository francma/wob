#define WOB_FILE "font_stub.c"

#include "font.h"
#include "log.h"

struct wob_font {};

struct wob_font *
wob_font_create(const char *fpath)
{
	wob_log_error("STUB wob_font_manager_load_font!");

	return NULL;
}

void
wob_font_destroy(struct wob_font *font)
{
	// intentionally left blank
}

struct wob_rendered_text_dimensions
wob_font_render_text_dimensions(struct wob_font *font, char *text, uint8_t font_size)
{
	wob_log_error("STUB wob_font_render_text_dimensions!");

	struct wob_rendered_text_dimensions d = {.h = 0, .w = 0};

	return d;
}

void
wob_font_render_text(struct wob_font *font, const char *text, int font_size, struct wob_color font_color, uint32_t *argb8888_buffer, size_t argb8888_buffer_size)
{
	wob_log_error("STUB wob_font_render_text!");
}

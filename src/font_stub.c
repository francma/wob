#define WOB_FILE "font_stub.c"

#include "font.h"
#include "log.h"

struct wob_font {};

struct wob_font_manager {};

struct wob_font_manager *
wob_font_manager_create()
{
	return NULL;
}

void
wob_font_manager_destroy(struct wob_font_manager *manager)
{
}

void
wob_font_manager_load_font(struct wob_font_manager *manager, const char *fpath)
{
	wob_log_error("STUB!");
}

struct wob_font *
wob_font_manager_get(struct wob_font_manager *manager, const char *fpath)
{
	wob_log_error("STUB!");

	return NULL;
}

struct wob_font_text_dimensions
wob_font_render_text_dimensions(struct wob_font *font, char *text, int font_size)
{
	wob_log_error("STUB!");

	struct wob_font_text_dimensions d = {.h = 0, .w = 0};

	return d;
}

void
wob_font_render_text(struct wob_font *font, char *text, int font_size, struct wob_color font_color, uint32_t *argb8888_buffer, size_t argb8888_buffer_size)
{
	wob_log_error("STUB!");
}

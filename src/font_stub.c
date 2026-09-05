#define WOB_FILE "font_stub.c"

#include "font.h"
#include "log.h"

struct wob_font {};

struct wob_font *
wob_font_create(const char *fpath)
{
	wob_log_warn("cannot load font at path %s as wob was compiled without font support, no text will render", fpath);

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
	wob_log_panic("wob was compiled without font support");
}

void
wob_font_render_text(struct wob_font *font, const char *text, int font_size, struct wob_color font_color, uint32_t *argb8888_buffer, size_t argb8888_buffer_size)
{
	wob_log_panic("wob was compiled without font support");
}

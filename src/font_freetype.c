#define WOB_FILE "font_freetype.c"

#include "font.h"
#include "log.h"

#include <ft2build.h>
#include <wayland-util.h>
#include FT_FREETYPE_H
#include FT_GLYPH_H

static bool library_initialized = false;
static FT_Library library;

struct wob_font {
	char *name;
	FT_Face data;
};

void
draw_glyph(uint32_t *pixels, size_t stride, FT_Bitmap *ft_bitmap, struct wob_color font_color)
{
	for (unsigned int row = 0; row < ft_bitmap->rows; row += 1) {
		for (unsigned int width = 0; width < ft_bitmap->width; width += 1) {
			uint8_t alpha = ft_bitmap->buffer[row * ft_bitmap->width + width];

			struct wob_color background = wob_color_from_argb8888(pixels[width]);
			struct wob_color foreground = font_color;

			foreground.a = (float) alpha / 255.0f;
			foreground = wob_color_premultiply_alpha(foreground);

			struct wob_color result = wob_color_blend_premultiplied(foreground, background);

			pixels[width] = wob_color_to_argb(result);
		}
		pixels += stride;
	}
}

struct wob_font *wob_font_create(const char *fpath)
{
	if (!library_initialized) {
		FT_Init_FreeType(&library);
	}

	struct wob_font *font = calloc(1, sizeof(struct wob_font));
	font->name = strdup(fpath);
	FT_New_Face(library, fpath, 0, &font->data);

	return font;
}

void wob_font_destroy(struct wob_font *font)
{
	free(font->name);
	FT_Done_Face(font->data);

	free(font);
}

struct wob_rendered_text_dimensions
wob_font_render_text_dimensions(struct wob_font *font, char *text, uint8_t font_size)
{
	struct wob_rendered_text_dimensions dimensions = {.h = 0, .w = 0};

	FT_Face ft_face = font->data;
	FT_Set_Pixel_Sizes(ft_face, 0, font_size);
	FT_UInt previous = 0;

	bool has_kerning = FT_HAS_KERNING(ft_face);

	for (char *c = text; *c != '\0'; c += 1) {
		FT_UInt glyph_index = FT_Get_Char_Index(ft_face, *c);
		FT_Load_Glyph(ft_face, glyph_index, FT_LOAD_DEFAULT);

		FT_Glyph glyph;
		FT_Get_Glyph(ft_face->glyph, &glyph);

		FT_BBox glyph_bbox;
		FT_Glyph_Get_CBox(glyph, ft_glyph_bbox_pixels, &glyph_bbox);
		FT_Vector delta;

		dimensions.h = glyph_bbox.yMax;
		dimensions.w += glyph_bbox.xMax;

		if (has_kerning && previous != 0) {
			FT_Get_Kerning(ft_face, previous, glyph_index, FT_KERNING_DEFAULT, &delta);
			dimensions.w += delta.x / 64;
		}

		previous = glyph_index;
	}

	return dimensions;
}

void
wob_font_render_text(struct wob_font *font, const char *text, int font_size, struct wob_color font_color, uint32_t *argb8888_buffer, size_t argb8888_buffer_stride)
{
	FT_Face ft_face = font->data;
	FT_UInt previous = 0;
	FT_Set_Pixel_Sizes(ft_face, 0, font_size);

	bool has_kerning = FT_HAS_KERNING(ft_face);

	for (const char *c = text; *c != '\0'; c += 1) {
		FT_Vector delta;
		FT_UInt glyph_index = FT_Get_Char_Index(ft_face, *c);
		FT_Load_Glyph(ft_face, glyph_index, FT_LOAD_DEFAULT);
		FT_Render_Glyph(ft_face->glyph, FT_RENDER_MODE_NORMAL);

		draw_glyph(argb8888_buffer, argb8888_buffer_stride, &ft_face->glyph->bitmap, font_color);

		argb8888_buffer += ft_face->glyph->advance.x / 64;
		if (has_kerning && previous != 0) {
			FT_Get_Kerning(ft_face, previous, glyph_index, FT_KERNING_DEFAULT, &delta);
			argb8888_buffer += delta.x / 64;
		}

		previous = glyph_index;
	}
}

#define WOB_FILE "font_freetype.c"

#include "font.h"
#include "log.h"

#include <ft2build.h>
#include <wayland-util.h>
#include FT_FREETYPE_H
#include FT_GLYPH_H

static int font_counter = 0;
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

struct wob_font *
wob_font_create(const char *fpath)
{
	FT_Error error;

	if (font_counter == 0) {
		error = FT_Init_FreeType(&library);
		if (error != 0) {
			wob_log_panic("FT_Init_FreeType failed, code = %d, error = %s", error, FT_Error_String(error));
		}
		font_counter += 1;
	}

	struct wob_font *font = calloc(1, sizeof(struct wob_font));
	font->name = strdup(fpath);

	error = FT_New_Face(library, fpath, 0, &font->data);
	if (error != 0) {
		wob_log_error("FT_New_Face failed, code = %d, error = %s", error, FT_Error_String(error));
		wob_font_destroy(font);

		return NULL;
	}

	return font;
}

void
wob_font_destroy(struct wob_font *font)
{
	FT_Error error;

	free(font->name);
	if (font->data != NULL) {
		error = FT_Done_Face(font->data);
		if (error != 0) {
			wob_log_panic("FT_Done_Face failed, code = %d, error = %s", error, FT_Error_String(error));
		}
	}

	free(font);

	font_counter -= 1;
	if (font_counter == 0) {
		error = FT_Done_FreeType(library);
		if (error != 0) {
			wob_log_panic("FT_Done_FreeType failed, code = %d, error = %s", error, FT_Error_String(error));
		}
	}
}

struct wob_rendered_text_dimensions
wob_font_render_text_dimensions(struct wob_font *font, char *text, uint8_t font_size)
{
	FT_Error error;

	FT_Face ft_face = font->data;
	error = FT_Set_Pixel_Sizes(ft_face, 0, font_size);
	if (error != 0) {
		wob_log_panic("FT_Set_Pixel_Sizes failed, code = %d, error = %s", error, FT_Error_String(error));
	}

	bool has_kerning = FT_HAS_KERNING(ft_face);
	FT_UInt previous = 0;
	struct wob_rendered_text_dimensions dimensions = {.h = 0, .w = 0};
	for (char *c = text; *c != '\0'; c += 1) {
		FT_UInt glyph_index = FT_Get_Char_Index(ft_face, *c);
		if (glyph_index == 0) {
			wob_log_panic("FT_Get_Char_Index failed, code = %d, error = %s", error, FT_Error_String(error));
		}

		error = FT_Load_Glyph(ft_face, glyph_index, FT_LOAD_DEFAULT);
		if (error != 0) {
			wob_log_panic("FT_Load_Glyph failed, code = %d, error = %s", error, FT_Error_String(error));
		}

		FT_Glyph glyph;
		FT_Get_Glyph(ft_face->glyph, &glyph);
		if (error != 0) {
			wob_log_panic("FT_Get_Glyph failed, code = %d, error = %s", error, FT_Error_String(error));
		}

		FT_BBox glyph_bbox;
		FT_Glyph_Get_CBox(glyph, ft_glyph_bbox_pixels, &glyph_bbox);
		FT_Vector delta;

		dimensions.h = glyph_bbox.yMax;
		dimensions.w += glyph_bbox.xMax;

		if (has_kerning && previous != 0) {
			error = FT_Get_Kerning(ft_face, previous, glyph_index, FT_KERNING_DEFAULT, &delta);
			if (error != 0) {
				wob_log_panic("FT_Get_Kerning failed, code = %d, error = %s", error, FT_Error_String(error));
			}
			dimensions.w += delta.x / 64;
		}

		previous = glyph_index;

		FT_Done_Glyph(glyph);
	}

	return dimensions;
}

void
wob_font_render_text(struct wob_font *font, const char *text, int font_size, struct wob_color font_color, uint32_t *argb8888_buffer, size_t argb8888_buffer_stride)
{
	FT_Error error;

	FT_Face ft_face = font->data;
	FT_UInt previous = 0;
	error = FT_Set_Pixel_Sizes(ft_face, 0, font_size);
	if (error != 0) {
		wob_log_panic("FT_Set_Pixel_Sizes failed, code = %d, error = %s", error, FT_Error_String(error));
	}

	bool has_kerning = FT_HAS_KERNING(ft_face);

	for (const char *c = text; *c != '\0'; c += 1) {
		FT_Vector delta;
		FT_UInt glyph_index = FT_Get_Char_Index(ft_face, *c);
		if (glyph_index == 0) {
			wob_log_panic("FT_Get_Char_Index failed, code = %d, error = %s", error, FT_Error_String(error));
		}
		error = FT_Load_Glyph(ft_face, glyph_index, FT_LOAD_DEFAULT);
		if (error != 0) {
			wob_log_panic("FT_Load_Glyph failed, code = %d, error = %s", error, FT_Error_String(error));
		}
		error = FT_Render_Glyph(ft_face->glyph, FT_RENDER_MODE_NORMAL);
		if (error != 0) {
			wob_log_panic("FT_Render_Glyph failed, code = %d, error = %s", error, FT_Error_String(error));
		}

		draw_glyph(argb8888_buffer, argb8888_buffer_stride, &ft_face->glyph->bitmap, font_color);

		argb8888_buffer += ft_face->glyph->advance.x / 64;
		if (has_kerning && previous != 0) {
			error = FT_Get_Kerning(ft_face, previous, glyph_index, FT_KERNING_DEFAULT, &delta);
			if (error != 0) {
				wob_log_panic("FT_Get_Kerning failed, code = %d, error = %s", error, FT_Error_String(error));
			}
			argb8888_buffer += delta.x / 64;
		}

		previous = glyph_index;
	}
}

#define WOB_FILE "font_freetype.c"

#include "font.h"
#include "log.h"

#include <wayland-util.h>
#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_GLYPH_H

FT_Library library;

struct wob_font {
	char *name;
	FT_Face data;
	struct wl_list link;
};

struct wob_font_manager {
	struct wl_list fonts;
};

void
draw_glyph(uint32_t *pixels, size_t stride, FT_Bitmap *ft_bitmap, struct wob_color font_color)
{
	for (unsigned int row = 0; row < ft_bitmap->rows; row += 1) {
		for (unsigned int width = 0; width < ft_bitmap->width; width += 1) {
			uint8_t alpha = ft_bitmap->buffer[row * ft_bitmap->width + width];

			struct wob_color background = wob_color_from_argb8888(pixels[width]);
			struct wob_color foreground = font_color;

			foreground.a = alpha / 255.0f;
			foreground = wob_color_premultiply_alpha(foreground);

			struct wob_color result = wob_color_blend_premultiplied(foreground, background);

			pixels[width] = wob_color_to_argb(result);
		}
		pixels += stride;
	}
}

struct wob_font_manager *
wob_font_manager_create()
{
	struct wob_font_manager *manager = malloc(sizeof(struct wob_font_manager));

	FT_Init_FreeType(&library);

	wl_list_init(&manager->fonts);

	return manager;
}

void
wob_font_manager_destroy(struct wob_font_manager *manager)
{
	struct wob_font *font, *font_tmp;
	wl_list_for_each_safe (font, font_tmp, &manager->fonts, link) {
		free(font->name);
		FT_Done_Face(font->data);

		free(font);
	}

	FT_Done_FreeType(library);
}

void
wob_font_manager_load_font(struct wob_font_manager *manager, const char *fpath)
{
	struct wob_font *font = calloc(1, sizeof(struct wob_font));
	font->name = strdup(fpath);
	FT_New_Face(library, fpath, 0, &font->data);

	wl_list_insert(&manager->fonts, &font->link);
}

struct wob_font *
wob_font_manager_get(struct wob_font_manager *manager, const char *fpath)
{
	struct wob_font *font;
	wl_list_for_each (font, &manager->fonts, link) {
		if (strcmp(font->name, fpath) == 0) {
			return font;
		}
	}

	return NULL;
}

struct wob_font_text_dimensions
wob_font_render_text_dimensions(struct wob_font *font, char *text, int font_size)
{
	struct wob_font_text_dimensions dimensions = {.h = 0, .w = 0};

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
			FT_Get_Kerning( ft_face, previous, glyph_index, FT_KERNING_DEFAULT, &delta);
			dimensions.w += delta.x / 64;
		}

		previous = glyph_index;
	}

	return dimensions;
}

void
wob_font_render_text(struct wob_font *font, char *text, int font_size, struct wob_color font_color, uint32_t *argb8888_buffer, size_t argb8888_buffer_size)
{
	FT_Face ft_face = font->data;
	FT_UInt previous = 0;
	FT_Set_Pixel_Sizes(ft_face, 0, font_size);

	bool has_kerning = FT_HAS_KERNING(ft_face);

	for (const char *c = text; *c != '\0'; c += 1) {
		FT_Vector  delta;
		FT_UInt glyph_index = FT_Get_Char_Index(ft_face, *c);
		FT_Load_Glyph(ft_face, glyph_index, FT_LOAD_DEFAULT);
		FT_Render_Glyph(ft_face->glyph, FT_RENDER_MODE_NORMAL);

		draw_glyph(argb8888_buffer, argb8888_buffer_size, &ft_face->glyph->bitmap, font_color);

		argb8888_buffer += ft_face->glyph->advance.x / 64;
		if (has_kerning && previous != 0) {
			FT_Get_Kerning( ft_face, previous, glyph_index, FT_KERNING_DEFAULT, &delta);
			argb8888_buffer += delta.x / 64;
		}

		previous = glyph_index;
	}
}

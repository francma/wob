#ifndef _WOB_COLOR_H
#define _WOB_COLOR_H

struct wob_color {
	float alpha;
	float red;
	float green;
	float blue;
};

uint32_t wob_color_to_argb(struct wob_color color);

struct wob_color wob_color_premultiply_alpha(struct wob_color color);

#endif
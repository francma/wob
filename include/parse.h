#ifndef _WOB_PARSE_H
#define _WOB_PARSE_H

#include <stdbool.h>
#include <stdint.h>

bool wob_parse_color(const char *restrict str, char **restrict str_end, uint32_t *color);

bool wob_parse_input(const char *input_buffer, unsigned long *percentage, uint32_t *background, uint32_t *border, uint32_t *bar);

#endif
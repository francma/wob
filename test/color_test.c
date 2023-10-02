#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <cmocka.h>

#include "src/color.h"

void
test_colors_are_case_insensitive(void **state)
{
	(void) state;
	struct wob_color color_a, color_b;
	assert_true(wob_color_from_rgba_string("ABCDEF", &color_a));
	assert_true(wob_color_from_rgba_string("abcdef", &color_b));

	assert_int_equal(color_a.r * UINT8_MAX, color_b.r * UINT8_MAX);
	assert_int_equal(color_a.g * UINT8_MAX, color_b.g * UINT8_MAX);
	assert_int_equal(color_a.b * UINT8_MAX, color_b.b * UINT8_MAX);
	assert_int_equal(color_a.a * UINT8_MAX, color_b.a * UINT8_MAX);
}

void
test_alpha_channel_is_optional(void **state)
{
	(void) state;
	struct wob_color color_a, color_b;
	assert_true(wob_color_from_rgba_string("AAAAAAFF", &color_a));
	assert_true(wob_color_from_rgba_string("AAAAAA", &color_b));

	assert_int_equal(color_a.r * UINT8_MAX, color_b.r * UINT8_MAX);
	assert_int_equal(color_a.g * UINT8_MAX, color_b.g * UINT8_MAX);
	assert_int_equal(color_a.b * UINT8_MAX, color_b.b * UINT8_MAX);
	assert_int_equal(color_a.a * UINT8_MAX, color_b.a * UINT8_MAX);
}

void
test_string_with_invalid_length_fails(void **state)
{
	(void) state;
	struct wob_color color;
	// too short
	assert_true(!wob_color_from_rgba_string("12345", &color));
	// alpha channel is incomplete
	assert_true(!wob_color_from_rgba_string("1234567", &color));
	// too long
	assert_true(!wob_color_from_rgba_string("123456789", &color));
}

void
test_string_with_invalid_characters_fails(void **state)
{
	(void) state;
	struct wob_color color;
	// whitespaces
	assert_true(!wob_color_from_rgba_string("  123456  ", &color));
}

void
test_valid_colors_from_string(void **state)
{
	(void) state;
	struct wob_color color;
	// without alpha
	assert_true(wob_color_from_rgba_string("123456", &color));
	assert_int_equal(color.r * UINT8_MAX, 0x12);
	assert_int_equal(color.g * UINT8_MAX, 0x34);
	assert_int_equal(color.b * UINT8_MAX, 0x56);
	assert_int_equal(color.a * UINT8_MAX, 0xFF);

	// with alpha
	assert_true(wob_color_from_rgba_string("12345678", &color));
	assert_int_equal(color.r * UINT8_MAX, 0x12);
	assert_int_equal(color.g * UINT8_MAX, 0x34);
	assert_int_equal(color.b * UINT8_MAX, 0x56);
	assert_int_equal(color.a * UINT8_MAX, 0x78);

	// hex characters
	assert_true(wob_color_from_rgba_string("ABCDEF", &color));
	assert_int_equal(color.r * UINT8_MAX, 0xAB);
	assert_int_equal(color.g * UINT8_MAX, 0xCD);
	assert_int_equal(color.b * UINT8_MAX, 0xEF);
}

int
main(void)
{
	const struct CMUnitTest tests[] = {
		cmocka_unit_test(test_colors_are_case_insensitive),
		cmocka_unit_test(test_alpha_channel_is_optional),
		cmocka_unit_test(test_string_with_invalid_length_fails),
		cmocka_unit_test(test_string_with_invalid_characters_fails),
		cmocka_unit_test(test_valid_colors_from_string),
	};

	return cmocka_run_group_tests(tests, NULL, NULL);
}

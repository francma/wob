#include <setjmp.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include <cmocka.h>

#include "src/readline.h"

void
test_number(void **state)
{
	(void) state;
	char *input = "30";
	char style[255];
	unsigned long value;

	assert_true(wob_readline(input, &value, style));
	assert_int_equal(value, 30);
	assert_string_equal(style, "\0");
}

void
test_zero(void **state)
{
	(void) state;
	char *input = "0";
	char style[255];
	unsigned long value;

	assert_true(wob_readline(input, &value, style));
	assert_int_equal(value, 0);
	assert_string_equal(style, "\0");
}

void
test_zero_with_style(void **state)
{
	(void) state;
	char *input = "0 muted";
	char style[255];
	unsigned long value;

	assert_true(wob_readline(input, &value, style));
	assert_int_equal(value, 0);
	assert_string_equal(style, "muted");
}

void
test_number_with_style(void **state)
{
	(void) state;
	char *input = "30 muted";
	char style[255];
	unsigned long value;

	assert_true(wob_readline(input, &value, style));
	assert_int_equal(value, 30);
	assert_string_equal(style, "muted");
}

void
test_number_with_empty_style(void **state)
{
	(void) state;
	char *input = "30 ";
	char style[255];
	unsigned long value;

	assert_false(wob_readline(input, &value, style));
}

int
main(void)
{
	const struct CMUnitTest tests[] = {
		cmocka_unit_test(test_number),
		cmocka_unit_test(test_zero),
		cmocka_unit_test(test_zero_with_style),
		cmocka_unit_test(test_number_with_style),
		cmocka_unit_test(test_number_with_empty_style),
	};

	return cmocka_run_group_tests(tests, NULL, NULL);
}

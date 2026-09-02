#include <ctype.h>
#include <stdbool.h>
#include <string.h>

enum states { S0, S00, S1, S2, S3, END };

#define GOTO(X)                                                                                                                                                                                        \
	s = X;                                                                                                                                                                                             \
	break

bool
wob_readline(char *input, unsigned long *out_value, char *out_style)
{
	unsigned long number = 0;
	char *style_start = NULL;

	enum states s = S0;
	for (char *c = input;; c++) {
		switch (s) {
			case S0:
				if (*c == '0') {
					number = 0;
					GOTO(S00);
				}
				if (isdigit(*c)) {
					number = number * 10 + (*c - '0');
					GOTO(S1);
				}

				return false;
			case S00:
				if (*c == ' ') {
					GOTO(S2);
				}
				if (*c == '\0') {
					GOTO(END);
				}

				return false;
			case S1:
				if (isdigit(*c)) {
					number = number * 10 + (*c - '0');
					GOTO(S1);
				}
				if (*c == '\0') {
					GOTO(END);
				}
				if (*c == ' ') {
					GOTO(S2);
				}

				return false;
			case S2:
				if (*c != '\0') {
					style_start = c;
					GOTO(S3);
				}

				return false;
			case S3:
				if (*c == '\0') {
					GOTO(END);
				}

				GOTO(S3);
			case END:
				*out_value = number;
				if (style_start != NULL) {
					strcpy(out_style, style_start);
				}
				else {
					out_style[0] = '\0';
				}

				return true;
		}
	}
}

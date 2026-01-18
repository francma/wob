#ifndef _WOB_READLINE_H
#define _WOB_READLINE_H

#include <stdbool.h>

bool wob_readline(const char *input, unsigned long *out_value, char *out_style);

#endif

#ifndef IO_H
#define IO_H

#include "common.h"
#include <stddef.h>

status_code get_input(const char *prompt, size_t max_len, char **input_out);

#endif

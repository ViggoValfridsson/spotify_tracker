#ifndef ENCODING_H
#define ENCODING_H

#include "common.h"

status_code base64_encode(const char *input, int input_len, char **base64_out, int *size_out);
status_code url_encode(const char *input, int input_len, char **url_encode_out);

#endif

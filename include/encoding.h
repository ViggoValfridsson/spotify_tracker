#ifndef ENCODING_H
#define ENCODING_H

int base64_encode(char *input, int input_len, char **base64_out);
int url_encode(char *input, int input_len, char **url_encode_out);

#endif

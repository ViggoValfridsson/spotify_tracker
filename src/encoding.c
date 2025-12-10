#include <curl/curl.h>
#include <curl/easy.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

#include "cencode.h"
#include "common.h"
#include "encoding.h"

int base64_encode(char *input, int input_len, char **base64_out) {
    base64_encodestate state;
    base64_init_encodestate(&state);
    int max_size = (input_len * 2) + 4;
    char *output = malloc(max_size);

    if (!output) {
        perror("malloc");
        return STATUS_ERROR;
    }

    int count = base64_encode_block(input, input_len, output, &state);
    count += base64_encode_blockend(output + count, &state);

    output[count] = '\0';
    *base64_out = output;
    return max_size;
}

int url_encode(char *input, int input_len, char **url_encode_out) {
    char *result = malloc(input_len * 3 + 1);
    int result_index = 0;

    if (!result) {
        perror("malloc");
        return STATUS_ERROR;
    }

    for (int i = 0; i < input_len; i++) {
        unsigned char c = input[i];

        if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '-' || c == '_' ||
            c == '.' || c == '~') {

            result[result_index] = c;
            result_index++;
        } else {
            static const char hex[] = "0123456789ABCDEF";
            result[result_index++] = '%';
            result[result_index++] = hex[c >> 4];
            result[result_index++] = hex[c & 0xF];
        }
    }

    result[result_index] = '\0';
    *url_encode_out = result;
    return STATUS_SUCCESS;
}

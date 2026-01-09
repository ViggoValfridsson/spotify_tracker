#include "encoding.h"
#include "cencode.h"
#include "common.h"
#include <curl/curl.h>
#include <curl/easy.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

int base64_encode(char *input, int input_len, char **base64_out, int *size_out) {
    int return_value = STATUS_ERROR;
    base64_encodestate state;
    base64_init_encodestate(&state);
    int max_size = (input_len * 2) + 4;

    char *output = malloc(max_size);
    if (!output) {
        perror("malloc");
        goto cleanup;
    }

    int count = base64_encode_block(input, input_len, output, &state);
    count += base64_encode_blockend(output + count, &state);

    output[count] = '\0';

    *base64_out = output;
    *size_out = count + 1;
    output = NULL;

    return_value = STATUS_SUCCESS;

cleanup:
    free(output);
    return return_value;
}

int url_encode(char *input, int input_len, char **url_encode_out) {
    int max_size = input_len * 3 + 1;
    int result_index = 0;

    char *result = malloc(max_size);
    if (!result) {
        perror("malloc");
        goto cleanup;
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
    result = NULL;

cleanup:
    free(result);
    return STATUS_SUCCESS;
}

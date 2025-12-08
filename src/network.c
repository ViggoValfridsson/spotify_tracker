#include <curl/curl.h>
#include <curl/easy.h>
#include <stdio.h>
#include <stdlib.h>

#include "cencode.h"
#include "common.h"
#include "network.h"

#define CREDENTIALS_MAX 1024
// Worst case scenario base64 growth
#define BASE64_CREDENTIALS_MAX ((CREDENTIALS_MAX) * 2 + 4)
// Make space for "Authorization: Basic  "
#define BASIC_HEADER_MAX (BASE64_CREDENTIALS_MAX + 22)

int base64_encode(char *input, int input_len, char **base64_out) {
    base64_encodestate state;
    base64_init_encodestate(&state);
    char *output = malloc(BASE64_CREDENTIALS_MAX);

    if (!output) {
        perror("malloc");
        return STATUS_ERROR;
    }

    int count = base64_encode_block(input, input_len, output, &state);
    count += base64_encode_blockend(output + count, &state);

    output[count] = '\0';
    *base64_out = output;
    return STATUS_SUCCESS;
}

int append_basic_header(char *username, char *password, struct curl_slist **header_out) {
    char credentials[CREDENTIALS_MAX];
    int snprint_res = snprintf(credentials, sizeof(credentials), "%s:%s", username, password);

    if (snprint_res >= CREDENTIALS_MAX) {
        fprintf(stderr, "Credentials are too long. Max length of password and username combined is %d\n",
                CREDENTIALS_MAX);
        return STATUS_ERROR;
    }

    char *base64_credentials;
    if (base64_encode(credentials, snprint_res, &base64_credentials)) {
        return STATUS_ERROR;
    }

    char basic_header[BASIC_HEADER_MAX];
    if (snprintf(basic_header, sizeof(basic_header), "Authorization: Basic %s", base64_credentials) >=
        BASIC_HEADER_MAX) {
        fprintf(stderr, "Basic header result is too long\n");
        return STATUS_ERROR;
    }

    free(base64_credentials);

    struct curl_slist *header = curl_slist_append(*header_out, basic_header);
    if (!header) {
        fprintf(stderr, "Failed to append header\n");
        return STATUS_ERROR;
    }

    *header_out = header;
    return STATUS_SUCCESS;
}

int post(char *url, struct curl_slist *headers) {
    // CURLcode *result;
    // CURL *curl = curl_easy_init();
    //
    // if (!curl) {
    //     return STATUS_NETWORK_ERROR;
    // }
    //
    // curl_easy_setopt(curl, CURLOPT_URL, url);
    // curl_easy_setopt(curl, CURLOPT_HEADER, headers);
}

#include <curl/curl.h>
#include <curl/easy.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

int get_status(int http_code) {
    if (http_code >= 200 && http_code <= 300) {
        return STATUS_SUCCESS;
    } else if (http_code == 401 || http_code == 403) {
        return STATUS_AUTHENTICATION_ERROR;
    } else if (http_code >= 300 && http_code <= 400) {
        return STATUS_NETWORK_CLIENT_ERROR;
    } else if (http_code >= 400 && http_code <= 500) {
        return STATUS_NETWORK_SERVER_ERROR;
    } else {
        return STATUS_BAD_HTTP_CODE;
    }
}

// TODO: implement returning response
int post(char *url, struct curl_slist *headers, const char *body, char **response_out) {
    CURL *curl = curl_easy_init();
    if (!curl) {
        fprintf(stderr, "Failed to initialize curl\n");
        return STATUS_NETWORK_ERROR;
    }

    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 15L);
    if (headers) {
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    }
    if (body) {
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, strlen(body));
    } else {
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, "");
        curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, 0L);
    }
    curl_easy_setopt(curl, CURLOPT_POST, 1L);

    CURLcode result = curl_easy_perform(curl);

    long http_code = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);

    if (result != CURLE_OK) {
        fprintf(stderr, "POST failed: %s\n", curl_easy_strerror(result));
    }

    curl_easy_cleanup(curl);

    if (result != CURLE_OK) {
        return STATUS_NETWORK_ERROR;
    }

    return get_status(http_code);
}

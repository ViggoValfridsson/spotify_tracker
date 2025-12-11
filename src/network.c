#include <curl/curl.h>
#include <curl/easy.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "encoding.h"
#include "network.h"

#define CREDENTIALS_MAX 1024

typedef struct {
    char *key;
    char *value;
} encoded_kvp;

int url_encode_kvp(form_key_value_pair *kvp, char **key_out, char **value_out, int *encoded_len_out) {
    char *encoded_key;
    char *encoded_value;
    int key_len = strlen(kvp->key);
    int value_len = strlen(kvp->value);

    if (url_encode(kvp->key, key_len, &encoded_key) != STATUS_SUCCESS) {
        fprintf(stderr, "Failed to url encode key\n");
        return STATUS_ERROR;
    }
    if (url_encode(kvp->value, value_len, &encoded_value) != STATUS_SUCCESS) {
        free(encoded_key);
        fprintf(stderr, "Failed to url encode value\n");
        return STATUS_ERROR;
    }

    *key_out = encoded_key;
    *value_out = encoded_value;
    *encoded_len_out = strlen(encoded_key) + strlen(encoded_value);
    return STATUS_SUCCESS;
}

void cleanup_encoded_kvps(encoded_kvp *encoded_kvps, int len) {
    if (!encoded_kvps) {
        return;
    }

    for (int i = 0; i < len; i++) {
        free(encoded_kvps[i].key);
        free(encoded_kvps[i].value);
    }

    free(encoded_kvps);
}

int url_encode_kvps(form_key_value_pair *kvps, int kvp_len, encoded_kvp **kvps_out, size_t *encoded_size_out) {
    encoded_kvp *encoded_kvps = malloc(sizeof(encoded_kvp) * kvp_len);
    size_t encoded_size = 0;

    if (!encoded_kvps) {
        return STATUS_ERROR;
    }

    for (int i = 0; i < kvp_len; i++) {
        int len;
        if (url_encode_kvp(&kvps[i], &encoded_kvps[i].key, &encoded_kvps[i].value, &len) != STATUS_SUCCESS) {
            // cleanup previous iterations
            cleanup_encoded_kvps(encoded_kvps, i);
            return STATUS_ERROR;
        }

        encoded_size += len;
        encoded_size += (i == 0 ? 1 : 2); // Account for '=' and/or '&='
    }

    *kvps_out = encoded_kvps;
    *encoded_size_out = encoded_size;
    return STATUS_SUCCESS;
}

void write_encoded_kvps_to_body(char *body, encoded_kvp *encoded_kvps, int kvp_len) {
    int position = 0;

    for (int i = 0; i < kvp_len; i++) {
        if (i > 0) {
            body[position++] = '&';
        }

        size_t key_len = strlen(encoded_kvps[i].key);
        size_t value_len = strlen(encoded_kvps[i].value);

        memcpy(body + position, encoded_kvps[i].key, key_len);
        position += key_len;

        body[position++] = '=';

        memcpy(body + position, encoded_kvps[i].value, value_len);
        position += value_len;
    }

    body[position] = '\0';
}

int create_form_url_encoded_body(form_key_value_pair *kvps, int kvp_len, char **body_out) {
    encoded_kvp *encoded_kvps = NULL;
    size_t encoded_size;

    if (url_encode_kvps(kvps, kvp_len, &encoded_kvps, &encoded_size) != STATUS_SUCCESS) {
        return STATUS_ERROR;
    }

    char *body = malloc(encoded_size + 1);

    if (!body) {
        perror("malloc");
        cleanup_encoded_kvps(encoded_kvps, kvp_len);
        return STATUS_ERROR;
    }

    write_encoded_kvps_to_body(body, encoded_kvps, kvp_len);

    *body_out = body;
    cleanup_encoded_kvps(encoded_kvps, kvp_len);

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
    int base64_size = base64_encode(credentials, snprint_res, &base64_credentials);
    if (base64_size == STATUS_ERROR) {
        return STATUS_ERROR;
    }

    // 22 to make space for "Authorization: Basic  "
    int header_len = base64_size + 22;
    char basic_header[header_len];
    if (snprintf(basic_header, sizeof(basic_header), "Authorization: Basic %s", base64_credentials) >= header_len) {
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

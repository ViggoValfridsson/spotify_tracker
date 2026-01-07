#include "network.h"
#include "cJSON.h"
#include "common.h"
#include "encoding.h"
#include <curl/curl.h>
#include <curl/easy.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CREDENTIALS_MAX 1024

typedef struct {
    char *key;
    char *value;
} encoded_kvp;

typedef struct {
    char *data;
    size_t size;
} response_chunks;

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

int create_form_url_encoded_kvps(form_key_value_pair *kvps, int kvp_len, char **body_out, size_t *size_out) {
    encoded_kvp *encoded_kvps = NULL;
    size_t encoded_size;

    if (url_encode_kvps(kvps, kvp_len, &encoded_kvps, &encoded_size) != STATUS_SUCCESS) {
        return STATUS_ERROR;
    }

    size_t real_size = encoded_size + 1;
    char *body = malloc(real_size);

    if (!body) {
        perror("malloc");
        cleanup_encoded_kvps(encoded_kvps, kvp_len);
        return STATUS_ERROR;
    }

    write_encoded_kvps_to_body(body, encoded_kvps, kvp_len);

    *body_out = body;
    *size_out = real_size;
    cleanup_encoded_kvps(encoded_kvps, kvp_len);

    return STATUS_SUCCESS;
}

int append_query_params(char *base_url, form_key_value_pair *parameters, int parameter_len, char **endpoint_out) {
    size_t encoded_params_len;
    char *encoded_parameters = NULL;

    int return_value =
        create_form_url_encoded_kvps(parameters, parameter_len, &encoded_parameters, &encoded_params_len);
    if (return_value != STATUS_SUCCESS) {
        goto cleanup;
    }

    // + 1 to account for ? separator before appending query params
    int endpoint_len = strlen(base_url) + 1 + encoded_params_len;

    char *endpoint = malloc(endpoint_len);
    if (!endpoint) {
        goto cleanup;
    }

    snprintf(endpoint, endpoint_len, "%s?%s", base_url, encoded_parameters);
    *endpoint_out = endpoint;
    return_value = STATUS_SUCCESS;

cleanup:
    free(encoded_parameters);
    return return_value;
}

int append_basic_header(char *username, char *password, struct curl_slist **header_out) {
    char credentials[CREDENTIALS_MAX];
    char *basic_header = NULL;
    char *base64_credentials = NULL;
    int snprint_res = snprintf(credentials, sizeof(credentials), "%s:%s", username, password);
    int return_value = STATUS_ERROR;

    if (snprint_res >= CREDENTIALS_MAX) {
        fprintf(stderr, "Credentials are too long. Max length of password and username combined is %d\n",
                CREDENTIALS_MAX);
        return_value = STATUS_ERROR;
        goto cleanup;
    }

    int base64_size;
    return_value = base64_encode(credentials, snprint_res, &base64_credentials, &base64_size);
    if (return_value != STATUS_SUCCESS) {
        fprintf(stderr, "Failed to base64 encode basic header\n");
        goto cleanup;
    }

    char basic_header_prefix[] = "Authorization: Basic ";
    int header_len = base64_size + strlen(basic_header_prefix) + 1;

    basic_header = malloc(header_len);
    if (!basic_header) {
        perror("malloc");
        return_value = STATUS_ERROR;
        goto cleanup;
    }

    if (snprintf(basic_header, header_len, "%s%s", basic_header_prefix, base64_credentials) >= header_len) {
        fprintf(stderr, "Basic header result is too long\n");
        return_value = STATUS_ERROR;
        goto cleanup;
    }

    struct curl_slist *header = curl_slist_append(*header_out, basic_header);
    if (!header) {
        fprintf(stderr, "Failed to append header\n");
        return_value = STATUS_ERROR;
        goto cleanup;
    }

    *header_out = header;
    return_value = STATUS_SUCCESS;

cleanup:
    free(base64_credentials);
    free(basic_header);

    return return_value;
}

int append_header(char *prefix, char *value, struct curl_slist **header_out) {
    int return_value = STATUS_ERROR;
    int prefix_len = strlen(prefix);
    int value_len = strlen(value);
    int total_size = prefix_len + value_len + 1;

    char *header_value = malloc(total_size);
    if (!header_value) {
        perror("malloc");
        goto cleanup;
    }

    if (snprintf(header_value, total_size, "%s%s", prefix, value) >= total_size) {
        fprintf(stderr, "Header result was too long\n");
        goto cleanup;
    }

    struct curl_slist *header = curl_slist_append(*header_out, header_value);
    if (!header) {
        fprintf(stderr, "Failed to append header\n");
        return STATUS_ERROR;
    }

    *header_out = header;
    return_value = STATUS_SUCCESS;

cleanup:
    free(header_value);
    return return_value;
}

int get_status(int http_code) {
    if (http_code >= 200 && http_code <= 300)
        return STATUS_SUCCESS;
    else if (http_code == 401 || http_code == 403)
        return STATUS_NETWORK_AUTHENTICATION_ERROR;
    else if (http_code == 404)
        return STATUS_NETWORK_NOT_FOUND_ERROR;
    else if (http_code >= 300 && http_code <= 400)
        return STATUS_NETWORK_CLIENT_ERROR;
    else if (http_code >= 400 && http_code <= 500)
        return STATUS_NETWORK_SERVER_ERROR;
    else
        return STATUS_BAD_HTTP_CODE;
}

int read_response_callback(void *contents, size_t size, size_t nmemb, void *userp) {
    size_t real_size = size * nmemb;
    response_chunks *chunks = (response_chunks *)userp;

    chunks->data = realloc(chunks->data, chunks->size + real_size + 1);
    if (!chunks->data) {
        perror("realloc");
        return STATUS_ERROR;
    }

    memcpy(&(chunks->data[chunks->size]), contents, real_size);
    chunks->size += real_size;
    chunks->data[chunks->size] = '\0';

    return real_size;
}

int http_request(char *url, struct curl_slist *headers, const char *body, char *method, char **response_out) {
    CURL *curl = curl_easy_init();
    if (!curl) {
        fprintf(stderr, "Failed to initialize curl\n");
        return STATUS_NETWORK_ERROR;
    }

    // Uncomment this to enable verbose logging
    // curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 15L);

    if (headers)
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

    if (body) {
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, strlen(body));
    } else {
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, NULL);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, 0L);
    }
    curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, method);

    response_chunks response_body = {.size = 0, .data = NULL};
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, read_response_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&response_body);

    CURLcode result = curl_easy_perform(curl);

    long http_code = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);

    if (result != CURLE_OK)
        fprintf(stderr, "%s failed: %s\n", method, curl_easy_strerror(result));

    curl_easy_cleanup(curl);
    if (response_body.data)
        *response_out = response_body.data;

    return result == CURLE_OK ? get_status(http_code) : STATUS_NETWORK_ERROR;
}

int parse_token_response(char *input, access_token **token_out) {
    cJSON *json = NULL;
    int return_value = STATUS_ERROR;

    json = cJSON_Parse(input);
    if (!json) {
        fprintf(stderr, "Response did not contain valid JSON\n");
        goto cleanup;
    }

    cJSON *access_token = cJSON_GetObjectItemCaseSensitive(json, "access_token");
    cJSON *refresh_token = cJSON_GetObjectItemCaseSensitive(json, "refresh_token");
    cJSON *token_type = cJSON_GetObjectItemCaseSensitive(json, "token_type");
    cJSON *expires_in = cJSON_GetObjectItemCaseSensitive(json, "expires_in");

    if (!cJSON_IsString(access_token) || !cJSON_IsString(token_type) || !cJSON_IsNumber(expires_in)) {
        fprintf(stderr, "Response was not in valid JSON structure\n");
        goto cleanup;
    }

    struct access_token *token = calloc(1, sizeof(struct access_token));
    if (token == NULL) {
        perror("calloc");
        goto cleanup;
    }

    snprintf(token->access_token, sizeof(token->access_token), "%s", access_token->valuestring);
    snprintf(token->token_type, sizeof(token->token_type), "%s", token_type->valuestring);
    token->expires_in = expires_in->valueint;
    // Refresh token is not always included, it being missing is not an error
    if (cJSON_IsString(refresh_token))
        snprintf(token->refresh_token, sizeof(token->refresh_token), "%s", refresh_token->valuestring);

    *token_out = token;
    return_value = STATUS_SUCCESS;

cleanup:
    cJSON_Delete(json);
    return return_value;
}

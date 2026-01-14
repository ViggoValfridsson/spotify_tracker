#ifndef NETWORK_H
#define NETWORK_H

#include <curl/curl.h>

typedef struct access_token {
    char access_token[512];
    char token_type[256];
    char refresh_token[512];
    int expires_in;
} access_token;

typedef struct {
    char client_id[256];
    char client_secret[256];
} client_credentials;

typedef struct {
    char key[256];
    char value[256];
} form_key_value_pair;

int append_header(const char *prefix, const char *value, struct curl_slist **header_out);
int append_basic_header(const char *username, const char *password, struct curl_slist **header_out);
int create_form_url_encoded_kvps(const form_key_value_pair *kvps, int kvp_len, char **body_out, size_t *size_out);
int append_query_params(const char *base_url, const form_key_value_pair *parameters, int parameter_len, char **endpoint_out);
int http_request(const char *url, const struct curl_slist *headers, const char *body, const char *method, char **response_out);
int parse_token_response(const char *input, access_token **token_out);

#endif

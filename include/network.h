#ifndef NETWORK_H
#define NETWORK_H

#include <curl/curl.h>

typedef struct {
    char access_token[512];
    char token_type[256];
    int expires_in;
} token_response;

typedef struct {
    char client_id[256];
    char client_secret[256];
} client_credentials;

typedef struct {
    char key[256];
    char value[256];
} form_key_value_pair;

int append_basic_header(char *username, char *password, struct curl_slist **header_out);
int create_form_url_encoded_body(form_key_value_pair *key_value_pairs,int kvp_len, char **body_out);
int post(char *url, struct curl_slist *headers, const char *body, char **response_out);

#endif

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

int append_basic_header(char *username, char *password, struct curl_slist **header_out);
int post(char *url, struct curl_slist *headers);

#endif

#include <curl/curl.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "common.h"
#include "file.h"
#include "io.h"
#include "network.h"
#include "spotify_api.h"

#define TOKEN_ENDPOINT "https://accounts.spotify.com/api/token"
#define AUTHORIZE_ENDPOINT_BASE "https://accounts.spotify.com/authorize"
// Includes null terminator
#define AUTHORIZE_ENDPOINT_BASE_LEN 40
#define AUTHORIZE_ENDPOINT_KVP_LEN 4
// This application is not listening to this url
#define REDIRECT_URI "https://httpbin.org/anything"

int get_token(token_response **token_out) {
    client_credentials *credentials = NULL;
    struct curl_slist *header = NULL;
    char *body = NULL;
    char *response = NULL;
    token_response *token = NULL;

    int return_value = read_credentials_from_file(CREDENTIALS_FILE_PATH, &credentials);
    if (return_value != STATUS_SUCCESS) {
        fprintf(stderr, "Failed to read credentials\n");
        goto cleanup;
    }

    return_value = append_basic_header(credentials->client_id, credentials->client_secret, &header);
    if (return_value != STATUS_SUCCESS) {
        fprintf(stderr, "Failed to create auth headers\n");
        goto cleanup;
    }

    form_key_value_pair grant_type = {.key = "grant_type", .value = "client_credentials"};
    size_t unused_len;

    return_value = create_form_url_encoded_kvps(&grant_type, 1, &body, &unused_len);
    if (return_value != STATUS_SUCCESS) {
        fprintf(stderr, "Failed to URL encode body\n");
        goto cleanup;
    }

    return_value = post(TOKEN_ENDPOINT, header, body, &response);
    if (return_value != STATUS_SUCCESS) {
        fprintf(stderr, "Failed to post token request\n");
        goto cleanup;
    }

    return_value = parse_token_response(response, &token);
    if (return_value != STATUS_SUCCESS) {
        fprintf(stderr, "Failed to parse token response\n");
    }

    return_value = STATUS_SUCCESS;
    *token_out = token;

cleanup:
    free(response);
    free(body);
    curl_slist_free_all(header);
    free(credentials);

    return return_value;
}

int create_authorization_endpoint(char **endpoint_out) {
    client_credentials *credentials = NULL;

    int return_value = read_credentials_from_file(CREDENTIALS_FILE_PATH, &credentials);
    if (return_value != STATUS_SUCCESS) {
        fprintf(stderr, "Failed to read credentials\n");
        goto cleanup;
    }

    form_key_value_pair query_parameters[AUTHORIZE_ENDPOINT_KVP_LEN] = {
        {"client_id", ""},
        {"response_type", "code"},
        {"redirect_uri", REDIRECT_URI},
        {"scope", "user-top-read"},
    };
    snprintf(query_parameters[0].value, sizeof(query_parameters[0].value), "%s", credentials->client_id);

    char *endpoint;
    return_value = append_query_params(AUTHORIZE_ENDPOINT_BASE, AUTHORIZE_ENDPOINT_BASE_LEN, query_parameters,
                                       AUTHORIZE_ENDPOINT_KVP_LEN, &endpoint);
    if (return_value != STATUS_SUCCESS) {
        goto cleanup;
    }

    *endpoint_out = endpoint;
    return_value = STATUS_SUCCESS;

cleanup:
    free(credentials);

    return return_value;
}

int login() {
    char *authorization_endpoint = NULL;
    char *redirect_code = NULL;
    char *redirect_state = NULL;
    token_response *token = NULL;
    int return_value = STATUS_ERROR;

    return_value = create_authorization_endpoint(&authorization_endpoint);
    if (return_value != STATUS_SUCCESS) {
        fprintf(stderr, "Failed to create authorization endpoint");
        goto cleanup;
    }

    int max_input_len = 255;

    printf("Please confirm the authentication: %s\n", authorization_endpoint);
    get_input("Enter \"code\" from the redirect URI query params\n", max_input_len, &redirect_code);
    get_input("Enter \"state\" from the redirect URI query params\n", max_input_len, &redirect_state);

    printf("%s\n", redirect_code);
    printf("%s\n", redirect_state);

    // TODO: maybe refactor this?
    if (get_token(&token) != STATUS_SUCCESS) {
        goto cleanup;
    }

    return_value = STATUS_SUCCESS;

cleanup:
    free(token);
    free(authorization_endpoint);
    free(redirect_code);
    free(redirect_state);

    return return_value;
}

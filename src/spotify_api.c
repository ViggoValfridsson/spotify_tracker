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

#define CREDENTIALS_FILE_PATH "~/.config/spotify-tracker/credentials"
#define REFRESH_TOKEN_FILE_PATH "~/.config/spotify-tracker/refresh_token"

#define TOKEN_ENDPOINT "https://accounts.spotify.com/api/token"
#define AUTHORIZE_ENDPOINT_BASE "https://accounts.spotify.com/authorize"
// Includes null terminator
#define AUTHORIZE_ENDPOINT_BASE_LEN 40
#define AUTHORIZE_PARAMETERS_LEN 4
// This application is not listening to this url
#define REDIRECT_URI "https://httpbin.org/anything"
#define TOKEN_ENDPOINT_FORM_BODY_LEN 3

int get_token(char *code, token_response **token_out) {
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
        fprintf(stderr, "Failed to create basic auth header\n");
        goto cleanup;
    }

    return_value = append_content_type_header("application/x-www-form-urlencoded", &header);
    if (return_value != STATUS_SUCCESS) {
        fprintf(stderr, "Failed to create content-type header\n");
        goto cleanup;
    }

    form_key_value_pair body_kvps[TOKEN_ENDPOINT_FORM_BODY_LEN] = {
        {"code", ""}, {"grant_type", "authorization_code"}, {"redirect_uri", REDIRECT_URI}};
    snprintf(body_kvps[0].value, sizeof(body_kvps[0].value), "%s", code);
    size_t unused_len;

    return_value = create_form_url_encoded_kvps(body_kvps, TOKEN_ENDPOINT_FORM_BODY_LEN, &body, &unused_len);
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
        goto cleanup;
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

    form_key_value_pair query_parameters[AUTHORIZE_PARAMETERS_LEN] = {
        {"client_id", ""},
        {"response_type", "code"},
        {"redirect_uri", REDIRECT_URI},
        {"scope", "user-top-read"},
    };
    snprintf(query_parameters[0].value, sizeof(query_parameters[0].value), "%s", credentials->client_id);

    char *endpoint;
    return_value = append_query_params(AUTHORIZE_ENDPOINT_BASE, AUTHORIZE_ENDPOINT_BASE_LEN, query_parameters,
                                       AUTHORIZE_PARAMETERS_LEN, &endpoint);
    if (return_value != STATUS_SUCCESS) {
        goto cleanup;
    }

    *endpoint_out = endpoint;
    return_value = STATUS_SUCCESS;

cleanup:
    free(credentials);

    return return_value;
}

int authorize_application(char **redirect_code_out) {
    char *authorization_endpoint = NULL;
    char *redirect_code = NULL;
    int return_value = STATUS_ERROR;

    return_value = create_authorization_endpoint(&authorization_endpoint);
    if (return_value != STATUS_SUCCESS) {
        fprintf(stderr, "Failed to create authorization endpoint");
        goto cleanup;
    }

    int max_input_len = 255;

    printf("Please confirm the authentication: %s\n", authorization_endpoint);
    return_value = get_input("Enter \"code\" from the redirect URI query params\n", max_input_len, &redirect_code);

    if (return_value != STATUS_SUCCESS) {
        fprintf(stderr, "Failed to read code from input\n");
        goto cleanup;
    }

    *redirect_code_out = redirect_code;
    return_value = STATUS_SUCCESS;

cleanup:
    free(authorization_endpoint);

    return return_value;
}

int login() {
    char *redirect_code = NULL;
    token_response *token = NULL;

    int return_value = authorize_application(&redirect_code);
    if (return_value != STATUS_SUCCESS) {
        goto cleanup;
    }

    return_value = get_token(redirect_code, &token);
    if (return_value != STATUS_SUCCESS) {
        goto cleanup;
    }

    return_value = write_file_overwrite(REFRESH_TOKEN_FILE_PATH, token->refresh_token);
    if (return_value != STATUS_SUCCESS) {
        goto cleanup;
    }

    return_value = STATUS_SUCCESS;

cleanup:
    free(token);
    free(redirect_code);

    return return_value;
}

int fetch_access_token() {
    char *refresh_token = NULL;
    int return_value = STATUS_ERROR;

    return_value = read_file_content(REFRESH_TOKEN_FILE_PATH, &refresh_token);
    if (return_value != STATUS_SUCCESS) {
        goto cleanup;
    }

    

cleanup:
    free(refresh_token);

    return return_value;
}

int get_top_artists(artist **artists_out) {

    // TODO:
    // check if refresh token file exists
    // get access token
    // fetch artists
    // parse artist json
    // return array of artists
    return STATUS_NOT_IMPLEMENTED;
}

int get_top_songs(artist **songs_out) {
    return STATUS_NOT_IMPLEMENTED;
}

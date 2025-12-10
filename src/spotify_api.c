#include <curl/curl.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "common.h"
#include "file.h"
#include "network.h"
#include "spotify_api.h"

#define AUTH_ENDPOINT "https://accounts.spotify.com/api/token"

int get_token() {
    client_credentials *credentials;

    if (read_credentials_from_file(CREDENTIALS_FILE_PATH, &credentials) != STATUS_SUCCESS) {
        fprintf(stderr, "Failed to read credentials\n");
        return STATUS_ERROR;
    }

    struct curl_slist *header = NULL;
    if (append_basic_header(credentials->client_id, credentials->client_secret, &header) != STATUS_SUCCESS) {
        fprintf(stderr, "Failed to create auth headers\n");
        return STATUS_ERROR;
    }

    char *body;
    form_key_value_pair grant_type = {
        .key = "grant_type",
        .value = "client_credentials"
    };

    if (create_form_url_encoded_body(&grant_type, 1, &body) != STATUS_SUCCESS) {
        fprintf(stderr, "Failed to URL encode body\n");
        free(credentials);
        return STATUS_ERROR;
    }

    // TODO: remove this body
    printf("Body %s\n", body);

    char *response;
    int post_status = post(AUTH_ENDPOINT, header, body, &response);

    if (post_status != STATUS_SUCCESS) {
        fprintf(stderr, "Failed to post token request\n");
        free(body);
        free(credentials);
        return post_status;
    }

    printf("http request was successful\n");

    // TODO: do we need to free as well *header, if so do it in all fail cases as well?
    // TODO: use goto to make freeing cleaner
    free(body);
    free(credentials);
    return STATUS_SUCCESS;
}

int get_top_artists() {
    get_token();

    return STATUS_SUCCESS;
}

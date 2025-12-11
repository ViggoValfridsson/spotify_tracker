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
    client_credentials *credentials = NULL;
    struct curl_slist *header = NULL;
    char *body = NULL;
    char *response = NULL;
    int return_value = STATUS_ERROR;

    return_value = read_credentials_from_file(CREDENTIALS_FILE_PATH, &credentials);
    if (return_value != STATUS_SUCCESS) {
        fprintf(stderr, "Failed to read credentials\n");
        goto cleanup;
    }

    return_value = append_basic_header(credentials->client_id, credentials->client_secret, &header);
    if (return_value != STATUS_SUCCESS) {
        fprintf(stderr, "Failed to create auth headers\n");
        goto cleanup;
    }

    form_key_value_pair grant_type = {
        .key = "grant_type",
        .value = "client_credentials"
    };

    return_value = create_form_url_encoded_body(&grant_type, 1, &body);
    if (return_value != STATUS_SUCCESS) {
        fprintf(stderr, "Failed to URL encode body\n");
        goto cleanup;
    }

    return_value = post(AUTH_ENDPOINT, header, body, &response);
    if (return_value != STATUS_SUCCESS) {
        fprintf(stderr, "Failed to post token request\n");
        goto cleanup;
    }

    return_value = STATUS_SUCCESS;

cleanup:
    // TODO: Uncomment this once post actually allocates response
    // free(response);
    free(body);
    curl_slist_free_all(header);
    free(credentials);

    return return_value;
}

int get_top_artists() {
    get_token();

    return STATUS_SUCCESS;
}

#include <curl/curl.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>

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
    append_basic_header(credentials->client_id, credentials->client_secret, &header);

    // TODO: remove these prints
    printf("%s\n", credentials->client_id);
    printf("%s\n", credentials->client_secret);
    printf("%s\n", header->data);

    char *body; /* TODO implent this to set grant type. = get_form_url_encoded_body(); */

    char *response;
    if (post(AUTH_ENDPOINT, header, body, &response) != STATUS_SUCCESS) {
        // TODO: remove this printf
        printf("Post failed\n");
    } else {
        printf("http request was successful\n");
    }

    free(credentials);
    return STATUS_SUCCESS;
}

int get_top_artists() {
    get_token();

    return STATUS_SUCCESS;
}

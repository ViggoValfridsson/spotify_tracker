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
    

    printf("%s\n", credentials->client_id);
    printf("%s\n", credentials->client_secret);
    printf("%s\n", header->data);

    free(credentials);
    return STATUS_SUCCESS;
}

int get_top_artists() {
    get_token();

    return STATUS_SUCCESS;
}

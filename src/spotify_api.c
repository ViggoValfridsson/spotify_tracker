#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>

#include "common.h"
#include "file.h"
#include "spotify_api.h"

typedef struct {
    // TODO: check if this is enough length
    char access_token[256];
    char token_type[256];
    int expires_in;
} token_response;

typedef struct {
    char client_id[256];
    char client_secret[256];
} credentials;

int read_credentials() {
    char *file_content;

    if (read_file_content(CREDENTIALS_FILE_PATH, &file_content) != STATUS_SUCCESS) {
        fprintf(stderr, "something went wrong");
        return STATUS_FILE_ERROR;
    }

    printf("%s", file_content);
    free(file_content);
    return STATUS_SUCCESS;
}

int get_top_artists() {
    read_credentials();

    return STATUS_SUCCESS;
}

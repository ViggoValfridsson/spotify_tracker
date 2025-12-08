#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>

#include "cJSON.h"
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
} credentials_file;

int read_credentials_json(char *file_content, credentials_file **credentials_out) {
    cJSON *json = cJSON_Parse(file_content);

    if (!json) {
        fprintf(stderr, "File did not contain valid JSON\n");
        return STATUS_FILE_ERROR;
    }

    cJSON *client_id = cJSON_GetObjectItemCaseSensitive(json, "clientId");
    cJSON *client_secret = cJSON_GetObjectItemCaseSensitive(json, "clientSecret");

    if (!cJSON_IsString(client_id) || !cJSON_IsString(client_secret)) {
        cJSON_Delete(json);
        return STATUS_FILE_ERROR;
    }

    credentials_file *credentials = malloc(sizeof(credentials_file));

    if (credentials == NULL) {
        cJSON_Delete(json);
        perror("malloc");
        return STATUS_ERROR;
    }

    snprintf(credentials->client_id, sizeof(credentials->client_id), "%s", client_id->valuestring);
    snprintf(credentials->client_secret, sizeof(credentials->client_secret), "%s", client_secret->valuestring);

    cJSON_Delete(json);

    *credentials_out = credentials;
    return STATUS_SUCCESS;
}

int read_credentials(char *credentials_file_path, credentials_file **credentials_out) {
    char *file_content;

    if (read_file_content(credentials_file_path, &file_content) != STATUS_SUCCESS) {
        return STATUS_FILE_ERROR;
    }

    credentials_file *credentials;

    if (read_credentials_json(file_content, &credentials) != STATUS_SUCCESS) {
        free(file_content);
        return STATUS_FILE_ERROR;
    }

    free(file_content);

    *credentials_out = credentials;
    return STATUS_SUCCESS;
}

int ensure_authenticated() {
    credentials_file *credentials;

    if (read_credentials(CREDENTIALS_FILE_PATH, &credentials) != STATUS_SUCCESS) {
        fprintf(stderr, "Failed to read credentials\n");
        return STATUS_ERROR;
    }

    printf("%s\n", credentials->client_id);
    printf("%s\n", credentials->client_secret);

    free(credentials);
    return STATUS_SUCCESS;
}

int get_top_artists() {
    ensure_authenticated();

    return STATUS_SUCCESS;
}

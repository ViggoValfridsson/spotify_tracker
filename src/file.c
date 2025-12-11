#include <curl/curl.h>
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>

#include "cJSON.h"
#include "common.h"
#include "file.h"
#include "network.h"

#define MAX_FILE_PATH 2048

void print_fopen_error() {
    if (errno == ENOENT) {
        fprintf(stderr, "Cannot read credentials file: File doesn't exist\n");
    } else if (errno == EACCES) {
        fprintf(stderr, "Cannot read credentials file: Permission denied\n");
    } else {
        fprintf(stderr, "Cannot read credentials file: Error %d\n", errno);
    }
}

// Expand ~ to actual home directory path
int expand_file_name(char *file_name, char **resolved_name_out) {
    if (file_name[0] != '~') {
        *resolved_name_out = file_name;
        return STATUS_SUCCESS;
    }

    char *home = getenv("HOME");

    if (!home) {
        fprintf(stderr, "Home directory environment variable not set\n");
        return STATUS_ERROR;
    }

    char *resolved_name = malloc(MAX_FILE_PATH);
    int result = snprintf(resolved_name, MAX_FILE_PATH, "%s%s", home, file_name + 1);

    if (result >= MAX_FILE_PATH) {
        fprintf(stderr, "Credentials file path is too long. Max path length is %d\n", MAX_FILE_PATH);
        return STATUS_ERROR;
    }
    if (result < 0) {
        perror("snprintf");
        return STATUS_ERROR;
    }

    *resolved_name_out = resolved_name;
    return STATUS_SUCCESS;
}

int open_file(char *file_name, FILE **file_out) {
    errno = 0;

    char *resolved_name;
    if (expand_file_name(file_name, &resolved_name) != STATUS_SUCCESS) {
        return STATUS_ERROR;
    }

    FILE *file = fopen(resolved_name, "r");

    if (!file) {
        free(resolved_name);
        print_fopen_error();
        return STATUS_FILE_ERROR;
    }

    free(resolved_name);
    *file_out = file;
    return STATUS_SUCCESS;
}

int get_file_size(FILE *file, long *size_out) {
    if (fseek(file, 0, SEEK_END) == -1) {
        perror("fseek");
        return STATUS_FILE_ERROR;
    }

    long size = ftell(file);
    if (size == -1) {
        perror("ftell");
        return STATUS_FILE_ERROR;
    }

    if (fseek(file, 0, SEEK_SET) == -1) {
        perror("fseek");
        return STATUS_FILE_ERROR;
    }

    *size_out = size;
    return STATUS_SUCCESS;
}

int read_file(FILE *file, long size, char **data_out) {
    char *data = malloc(size + 1);

    if (data == NULL) {
        fclose(file);
        perror("malloc");
        return STATUS_ERROR;
    }

    if (fread(data, 1, size, file) == 0 && ferror(file) != 0) {
        fclose(file);
        free(data);
        perror("fread");
        return STATUS_FILE_ERROR;
    }

    fclose(file);
    data[size] = '\0';
    *data_out = data;
    return STATUS_SUCCESS;
}

int read_file_content(char *file_path, char **file_content) {
    FILE *file = NULL;
    if (open_file(file_path, &file) != STATUS_SUCCESS) {
        return STATUS_FILE_ERROR;
    }

    long size;
    if (get_file_size(file, &size)) {
        return STATUS_FILE_ERROR;
    }

    char *data;
    if (read_file(file, size, &data)) {
        return STATUS_FILE_ERROR;
    }

    *file_content = data;
    return STATUS_SUCCESS;
}

int read_credentials_json(char *file_content, client_credentials **credentials_out) {
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

    client_credentials *credentials = malloc(sizeof(client_credentials));

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

int read_credentials_from_file(char *credentials_file_path, client_credentials **credentials_out) {
    char *file_content;

    if (read_file_content(credentials_file_path, &file_content) != STATUS_SUCCESS) {
        return STATUS_FILE_ERROR;
    }

    client_credentials *credentials;

    if (read_credentials_json(file_content, &credentials) != STATUS_SUCCESS) {
        free(file_content);
        return STATUS_FILE_ERROR;
    }

    free(file_content);

    *credentials_out = credentials;
    return STATUS_SUCCESS;
}

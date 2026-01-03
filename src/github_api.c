#include "github_api.h"
#include "cJSON.h"
#include "common.h"
#include "file.h"
#include "network.h"
#include <curl/curl.h>
#include <stdio.h>
#include <stdlib.h>

#define CONFIG_FILE_PATH "~/.config/spotify-tracker/github-config"
#define GITHUB_API_BASE_ADDRESS "https://api.github.com"

typedef struct {
    char gist_id[256];
    char file_name[256];
    char pat[256];
} github_config;

int parse_config_json(char *file_content, github_config **config_out) {
    cJSON *json = cJSON_Parse(file_content);

    if (!json) {
        fprintf(stderr, "File did not contain valid JSON\n");
        return STATUS_FILE_ERROR;
    }

    cJSON *gist_id = cJSON_GetObjectItemCaseSensitive(json, "gistId");
    cJSON *file_name = cJSON_GetObjectItemCaseSensitive(json, "fileName");
    cJSON *pat = cJSON_GetObjectItemCaseSensitive(json, "pat");

    if (!cJSON_IsString(gist_id) || !cJSON_IsString(file_name) || !cJSON_IsString(pat)) {
        cJSON_Delete(json);
        return STATUS_FILE_ERROR;
    }

    github_config *config = malloc(sizeof(github_config));

    if (config == NULL) {
        cJSON_Delete(json);
        perror("malloc");
        return STATUS_ERROR;
    }

    snprintf(config->gist_id, sizeof(config->gist_id), "%s", gist_id->valuestring);
    snprintf(config->file_name, sizeof(config->file_name), "%s", file_name->valuestring);
    snprintf(config->pat, sizeof(config->pat), "%s", pat->valuestring);

    cJSON_Delete(json);

    *config_out = config;
    return STATUS_SUCCESS;
}

int read_github_config_file(char *file_path, github_config **config_out) {
    char *file_content;

    if (read_file_content(file_path, &file_content) != STATUS_SUCCESS) {
        return STATUS_FILE_ERROR;
    }

    github_config *config;

    if (parse_config_json(file_content, &config) != STATUS_SUCCESS) {
        free(file_content);
        return STATUS_FILE_ERROR;
    }

    free(file_content);

    *config_out = config;
    return STATUS_SUCCESS;
}

int create_update_gist_body(char *description, char *file_name, char *gist_content, char **json_out) {
    cJSON *root = cJSON_CreateObject();
    if (!root) {
        return STATUS_ERROR;
    }
    if (!cJSON_AddStringToObject(root, "description", description)) {
        cJSON_Delete(root);
        return STATUS_ERROR;
    }
    cJSON *files = cJSON_CreateObject();
    if (!files) {
        cJSON_Delete(root);
        return STATUS_ERROR;
    }
    cJSON *file = cJSON_CreateObject();
    if (!file) {
        cJSON_Delete(root);
        return STATUS_ERROR;
    }
    if (!cJSON_AddStringToObject(file, "content", gist_content)) {
        cJSON_Delete(root);
        return STATUS_ERROR;
    }
    if (!cJSON_AddItemToObject(files, file_name, file)) {
        cJSON_Delete(root);
        return STATUS_ERROR;
    }
    if (!cJSON_AddItemToObject(root, "files", files)) {
        cJSON_Delete(root);
        return STATUS_ERROR;
    }

    char *json_string = cJSON_Print(root);

    cJSON_Delete(root);
    if (!json_string) {
        return STATUS_ERROR;
    }

    *json_out = json_string;
    return STATUS_SUCCESS;
}

int update_gist_content(char *content) {
    github_config *config = NULL;
    struct curl_slist *headers = NULL;
    char *update_endpoint = NULL;
    char *update_body = NULL;
    char *response = NULL;
    int return_value = STATUS_ERROR;

    return_value = read_github_config_file(CONFIG_FILE_PATH, &config);
    if (return_value != STATUS_SUCCESS) {
        fprintf(stderr, "Failed to parse github config file\n");
        goto cleanup;
    }

    if (asprintf(&update_endpoint, "%s/gists/%s", GITHUB_API_BASE_ADDRESS, config->gist_id) == -1) {
        perror("asprintf");
        goto cleanup;
    }

    return_value = create_update_gist_body("Spotify Stats", config->file_name, content, &update_body);
    if (return_value != STATUS_SUCCESS) {
        fprintf(stderr, "Failed to construct gist update json\n");
        goto cleanup;
    }

    return_value = append_header("Authorization: Bearer ", config->pat, &headers);
    if (return_value != STATUS_SUCCESS) {
        fprintf(stderr, "Failed to create bearer token header\n");
        goto cleanup;
    }

    return_value = append_header("Accept: ", "application/vnd.github+json", &headers);
    if (return_value != STATUS_SUCCESS) {
        fprintf(stderr, "Failed to create accept header\n");
        goto cleanup;
    }

    return_value = append_header("User-Agent: ", "Spotify Tracker", &headers);
    if (return_value != STATUS_SUCCESS) {
        fprintf(stderr, "Failed to create accept header\n");
        goto cleanup;
    }

    return_value = http_request(update_endpoint, headers, update_body, "PATCH", &response);
    if (return_value != STATUS_SUCCESS) {
        fprintf(stderr, "Failed to post update to github API\n");
        goto cleanup;
    }

    return_value = STATUS_SUCCESS;

cleanup:
    free(config);
    free(update_endpoint);
    free(update_body);
    free(response);
    curl_slist_free_all(headers);

    return return_value;
}

#include "github_api.h"
#include "cJSON.h"
#include "common.h"
#include "encoding.h"
#include "file.h"
#include "network.h"
#include <curl/curl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CONFIG_FILE_PATH "~/.config/spotify-tracker/github-config"
#define GITHUB_API_BASE_ADDRESS "https://api.github.com"

typedef struct {
    char repo_owner[256];
    char repo_name[256];
    char file_name[256];
    char committer[256];
    char email[256];
    char pat[256];
} github_config;

int parse_config_json(char *file_content, github_config **config_out) {
    cJSON *json = NULL;
    int return_value = STATUS_ERROR;

    json = cJSON_Parse(file_content);
    if (!json) {
        fprintf(stderr, "File did not contain valid JSON\n");
        goto cleanup;
    }

    cJSON *repo_owner = cJSON_GetObjectItemCaseSensitive(json, "repoOwner");
    cJSON *repo_name = cJSON_GetObjectItemCaseSensitive(json, "repoName");
    cJSON *file_name = cJSON_GetObjectItemCaseSensitive(json, "fileName");
    cJSON *committer = cJSON_GetObjectItemCaseSensitive(json, "committer");
    cJSON *email = cJSON_GetObjectItemCaseSensitive(json, "email");
    cJSON *pat = cJSON_GetObjectItemCaseSensitive(json, "pat");

    if (!cJSON_IsString(repo_owner) || !cJSON_IsString(repo_name) || !cJSON_IsString(file_name) ||
        !cJSON_IsString(committer) || !cJSON_IsString(email) || !cJSON_IsString(pat))
        goto cleanup;

    github_config *config = malloc(sizeof(github_config));
    if (config == NULL) {
        perror("malloc");
        goto cleanup;
    }

    snprintf(config->repo_owner, sizeof(config->repo_owner), "%s", repo_owner->valuestring);
    snprintf(config->repo_name, sizeof(config->repo_name), "%s", repo_name->valuestring);
    snprintf(config->file_name, sizeof(config->file_name), "%s", file_name->valuestring);
    snprintf(config->committer, sizeof(config->committer), "%s", committer->valuestring);
    snprintf(config->email, sizeof(config->email), "%s", email->valuestring);
    snprintf(config->pat, sizeof(config->pat), "%s", pat->valuestring);

    *config_out = config;
    return_value = STATUS_SUCCESS;

cleanup:
    cJSON_Delete(json);
    if (return_value != STATUS_SUCCESS)
        free(config);

    return return_value;
}

int read_github_config_file(char *file_path, github_config **config_out) {
    char *file_content = NULL;

    int return_value = read_file_content(file_path, &file_content);
    if (return_value != STATUS_SUCCESS)
        goto cleanup;

    github_config *config;

    return_value = parse_config_json(file_content, &config);
    if (return_value != STATUS_SUCCESS)
        goto cleanup;

    *config_out = config;
    return_value = STATUS_SUCCESS;

cleanup:
    free(file_content);
    return return_value;
}

int create_update_file_body(char *message, char *committer_name, char *committer_email, char *sha, char *content,
                            char **json_out) {
    char *base64_content = NULL;
    int return_value = STATUS_ERROR;

    cJSON *root = cJSON_CreateObject();
    if (!root)
        goto cleanup;

    if (!cJSON_AddStringToObject(root, "message", message))
        goto cleanup;

    cJSON *committer = cJSON_CreateObject();
    if (!committer)
        goto cleanup;

    if (!cJSON_AddStringToObject(committer, "name", committer_name))
        goto cleanup;

    if (!cJSON_AddStringToObject(committer, "email", committer_email))
        goto cleanup;

    if (!cJSON_AddItemToObject(root, "committer", committer))
        goto cleanup;

    if (sha && !cJSON_AddStringToObject(root, "sha", sha))
        goto cleanup;

    int content_len = strlen(content);
    int base64_size;
    return_value = base64_encode(content, content_len, &base64_content, &base64_size);
    if (return_value == STATUS_ERROR)
        goto cleanup;

    if (!cJSON_AddStringToObject(root, "content", base64_content))
        goto cleanup;

    char *json_string = cJSON_Print(root);
    if (!json_string)
        goto cleanup;

    *json_out = json_string;
    return_value = STATUS_SUCCESS;

cleanup:
    free(base64_content);
    cJSON_Delete(root);

    return return_value;
}

int parse_file_json(char *input, char **sha_out) {
    cJSON *json = cJSON_Parse(input);
    int return_value = STATUS_ERROR;

    if (!json) {
        fprintf(stderr, "File did not contain valid JSON\n");
        goto cleanup;
    }

    cJSON *sha_json = cJSON_GetObjectItemCaseSensitive(json, "sha");
    if (!cJSON_IsString(sha_json))
        goto cleanup;

    int sha_len = strlen(sha_json->valuestring) + 1;
    char *sha = malloc(sha_len);
    if (sha == NULL) {
        perror("malloc");
        goto cleanup;
    }

    snprintf(sha, sha_len, "%s", sha_json->valuestring);

    *sha_out = sha;
    return_value = STATUS_SUCCESS;

cleanup:
    cJSON_Delete(json);
    return return_value;
}

int get_existing_file_sha_headers(github_config *config, struct curl_slist **headers_out) {
    struct curl_slist *headers = NULL;

    int return_value = append_header("Accept: ", "application/vnd.github+json", &headers);
    if (return_value != STATUS_SUCCESS) {
        fprintf(stderr, "Failed to append Accept header\n");
        goto cleanup;
    }

    return_value = append_header("Authorization: Bearer ", config->pat, &headers);
    if (return_value != STATUS_SUCCESS) {
        fprintf(stderr, "Failed to create bearer token header\n");
        goto cleanup;
    }

    return_value = append_header("User-Agent: ", "Spotify Tracker", &headers);
    if (return_value != STATUS_SUCCESS) {
        fprintf(stderr, "Failed to create User-Agent header\n");
        goto cleanup;
    }

    *headers_out = headers;
    return_value = STATUS_SUCCESS;

cleanup:
    if (return_value != STATUS_SUCCESS)
        curl_slist_free_all(headers);

    return return_value;
}

int get_existing_file_sha(char *endpoint, github_config *config, char **sha_out) {
    struct curl_slist *headers = NULL;
    char *sha;
    char *response = NULL;

    int return_value = get_existing_file_sha_headers(config, &headers);
    if (return_value != STATUS_SUCCESS)
        goto cleanup;

    return_value = http_request(endpoint, headers, NULL, "GET", &response);
    // File was not found, this is not an error
    if (return_value == STATUS_NETWORK_NOT_FOUND_ERROR) {
        *sha_out = NULL;
        return_value = STATUS_SUCCESS;
        goto cleanup;
    } else if (return_value != STATUS_SUCCESS) {
        fprintf(stderr, "Failed to perform get request to Github API\n");
        goto cleanup;
    }

    return_value = parse_file_json(response, &sha);
    if (return_value != STATUS_SUCCESS) {
        fprintf(stderr, "Failed to extract sha from JSON response\n");
        goto cleanup;
    }

    *sha_out = sha;
    return_value = STATUS_SUCCESS;

cleanup:
    free(response);
    curl_slist_free_all(headers);

    return return_value;
}

int get_update_file_content_headers(github_config *config, struct curl_slist **headers_out) {
    struct curl_slist *headers = NULL;

    int return_value = append_header("Authorization: Bearer ", config->pat, &headers);
    if (return_value != STATUS_SUCCESS) {
        fprintf(stderr, "Failed to create bearer token header\n");
        goto cleanup;
    }

    return_value = append_header("Accept: ", "application/vnd.github+json", &headers);
    if (return_value != STATUS_SUCCESS) {
        fprintf(stderr, "Failed to create Accept header\n");
        goto cleanup;
    }

    return_value = append_header("User-Agent: ", "Spotify Tracker", &headers);
    if (return_value != STATUS_SUCCESS) {
        fprintf(stderr, "Failed to create User-Agent header\n");
        goto cleanup;
    }

    *headers_out = headers;
    return_value = STATUS_SUCCESS;

cleanup:
    if (return_value != STATUS_SUCCESS)
        curl_slist_free_all(headers);

    return return_value;
}

int update_repo_file_content(char *content) {
    github_config *config = NULL;
    struct curl_slist *headers = NULL;
    char *repo_endpoint = NULL;
    char *update_body = NULL;
    char *response = NULL;
    char *sha = NULL;
    int return_value = STATUS_ERROR;

    return_value = read_github_config_file(CONFIG_FILE_PATH, &config);
    if (return_value != STATUS_SUCCESS) {
        fprintf(stderr, "Failed to parse github config file\n");
        goto cleanup;
    }

    if (asprintf(&repo_endpoint, "%s/repos/%s/%s/contents/%s", GITHUB_API_BASE_ADDRESS, config->repo_owner,
                 config->repo_name, config->file_name) == -1) {
        perror("asprintf");
        goto cleanup;
    }

    return_value = get_existing_file_sha(repo_endpoint, config, &sha);
    if (return_value != STATUS_SUCCESS) {
        fprintf(stderr, "Failed to extract existing file SHA\n");
        goto cleanup;
    }

    char *commit_message = "Performed Spotify Tracker update";
    return_value =
        create_update_file_body(commit_message, config->committer, config->email, sha, content, &update_body);
    if (return_value != STATUS_SUCCESS) {
        fprintf(stderr, "Failed to construct file update json\n");
        goto cleanup;
    }

    return_value = get_update_file_content_headers(config, &headers);
    if (return_value != STATUS_SUCCESS)
        goto cleanup;

    return_value = http_request(repo_endpoint, headers, update_body, "PUT", &response);
    if (return_value != STATUS_SUCCESS) {
        fprintf(stderr, "Failed to post update to github API\n");
        goto cleanup;
    }

    return_value = STATUS_SUCCESS;

cleanup:
    free(config);
    free(repo_endpoint);
    free(update_body);
    free(response);
    free(sha);
    curl_slist_free_all(headers);

    return return_value;
}

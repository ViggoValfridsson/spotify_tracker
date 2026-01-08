#include "spotify_api.h"
#include "cJSON.h"
#include "common.h"
#include "file.h"
#include "io.h"
#include "network.h"
#include <curl/curl.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>

#define CREDENTIALS_FILE_PATH "~/.config/spotify-tracker/spotify-credentials"
#define REFRESH_TOKEN_FILE_PATH "~/.config/spotify-tracker/spotify-refresh-token"

#define TOKEN_ENDPOINT "https://accounts.spotify.com/api/token"
#define AUTHORIZE_ENDPOINT_BASE "https://accounts.spotify.com/authorize"
#define TOP_ARTISTS_ENDPOINT "https://api.spotify.com/v1/me/top/artists?time_range=short_term&limit=5&offset=0"
#define TOP_TRACKS_ENDPOINT "https://api.spotify.com/v1/me/top/tracks?time_range=short_term&limit=5&offset=0"
#define REDIRECT_URI "https://httpbin.org/anything"

int parse_credentials_json(char *file_content, client_credentials **credentials_out) {
    cJSON *json = NULL;
    int return_value = STATUS_ERROR;

    json = cJSON_Parse(file_content);
    if (!json) {
        fprintf(stderr, "File did not contain valid JSON\n");
        goto cleanup;
    }

    cJSON *client_id = cJSON_GetObjectItemCaseSensitive(json, "clientId");
    cJSON *client_secret = cJSON_GetObjectItemCaseSensitive(json, "clientSecret");

    if (!cJSON_IsString(client_id) || !cJSON_IsString(client_secret))
        goto cleanup;

    client_credentials *credentials = malloc(sizeof(client_credentials));
    if (credentials == NULL) {
        perror("malloc");
        goto cleanup;
    }

    snprintf(credentials->client_id, sizeof(credentials->client_id), "%s", client_id->valuestring);
    snprintf(credentials->client_secret, sizeof(credentials->client_secret), "%s", client_secret->valuestring);

    *credentials_out = credentials;
    return_value = STATUS_SUCCESS;

cleanup:
    cJSON_Delete(json);

    return return_value;
    return STATUS_SUCCESS;
}

int read_spotify_credentials_file(char *credentials_file_path, client_credentials **credentials_out) {
    client_credentials *credentials = NULL;
    char *file_content = NULL;

    int return_value = read_file_content(credentials_file_path, &file_content);
    if (return_value != STATUS_SUCCESS)
        goto cleanup;

    return_value = parse_credentials_json(file_content, &credentials);
    if (return_value != STATUS_SUCCESS)
        goto cleanup;

    *credentials_out = credentials;
    return_value = STATUS_SUCCESS;

cleanup:
    free(file_content);
    return return_value;
}

int get_access_token_header(struct curl_slist **header_out) {
    client_credentials *credentials = NULL;
    struct curl_slist *header = NULL;

    int return_value = read_spotify_credentials_file(CREDENTIALS_FILE_PATH, &credentials);
    if (return_value != STATUS_SUCCESS) {
        fprintf(stderr, "Failed to read credentials\n");
        goto cleanup;
    }

    return_value = append_basic_header(credentials->client_id, credentials->client_secret, &header);
    if (return_value != STATUS_SUCCESS) {
        fprintf(stderr, "Failed to create basic auth header\n");
        goto cleanup;
    }

    return_value = append_header("Content-Type: ", "application/x-www-form-urlencoded", &header);
    if (return_value != STATUS_SUCCESS) {
        fprintf(stderr, "Failed to create content-type header\n");
        goto cleanup;
    }

    *header_out = header;
    return_value = STATUS_SUCCESS;

cleanup:
    free(credentials);
    if (return_value != STATUS_SUCCESS)
        curl_slist_free_all(header);

    return return_value;
}

int get_access_token(char *body, access_token **token_out) {
    struct curl_slist *header = NULL;
    char *response = NULL;
    access_token *token = NULL;

    int return_value = get_access_token_header(&header);
    if (return_value != STATUS_SUCCESS) {
        fprintf(stderr, "Failed to construct HTTP header\n");
        goto cleanup;
    }

    return_value = http_request(TOKEN_ENDPOINT, header, body, "POST", &response);
    if (return_value != STATUS_SUCCESS) {
        fprintf(stderr, "Failed to post token request\n");
        goto cleanup;
    }

    return_value = parse_token_response(response, &token);
    if (return_value != STATUS_SUCCESS) {
        fprintf(stderr, "Failed to parse token response\n");
        goto cleanup;
    }

    *token_out = token;
    return_value = STATUS_SUCCESS;

cleanup:
    free(response);
    curl_slist_free_all(header);

    return return_value;
}

int get_access_token_from_authorization_code(char *redirect_code, access_token **token_out) {
    access_token *token = NULL;
    char *body = NULL;

    form_key_value_pair body_kvps[] = {
        {"code", ""}, {"grant_type", "authorization_code"}, {"redirect_uri", REDIRECT_URI}};
    snprintf(body_kvps[0].value, sizeof(body_kvps[0].value), "%s", redirect_code);
    size_t unused;

    int body_len = sizeof(body_kvps) / sizeof(body_kvps[0]);
    int return_value = create_form_url_encoded_kvps(body_kvps, body_len, &body, &unused);
    if (return_value != STATUS_SUCCESS) {
        fprintf(stderr, "Failed to URL encode body\n");
        goto cleanup;
    }

    return_value = get_access_token(body, &token);
    if (return_value != STATUS_SUCCESS)
        goto cleanup;

    *token_out = token;
    return_value = STATUS_SUCCESS;

cleanup:
    free(body);
    return return_value;
}

int get_access_token_from_refresh_token(char *refresh_token, access_token **token_out) {
    access_token *token = NULL;
    char *body = NULL;

    form_key_value_pair body_kvps[] = {{"refresh_token", ""}, {"grant_type", "refresh_token"}};
    snprintf(body_kvps[0].value, sizeof(body_kvps[0].value), "%s", refresh_token);
    size_t unused_len;

    int body_len = sizeof(body_kvps) / sizeof(body_kvps[0]);
    int return_value = create_form_url_encoded_kvps(body_kvps, body_len, &body, &unused_len);
    if (return_value != STATUS_SUCCESS) {
        fprintf(stderr, "Failed to URL encode body\n");
        goto cleanup;
    }

    return_value = get_access_token(body, &token);
    if (return_value != STATUS_SUCCESS)
        goto cleanup;

    *token_out = token;
    return_value = STATUS_SUCCESS;

cleanup:
    free(body);
    return return_value;
}

int create_authorization_endpoint(char **endpoint_out) {
    client_credentials *credentials = NULL;

    int return_value = read_spotify_credentials_file(CREDENTIALS_FILE_PATH, &credentials);
    if (return_value != STATUS_SUCCESS) {
        fprintf(stderr, "Failed to read credentials\n");
        goto cleanup;
    }

    form_key_value_pair query_parameters[] = {
        {"client_id", ""},
        {"response_type", "code"},
        {"redirect_uri", REDIRECT_URI},
        {"scope", "user-top-read"},
    };
    snprintf(query_parameters[0].value, sizeof(query_parameters[0].value), "%s", credentials->client_id);

    int parameters_len = sizeof(query_parameters) / sizeof(query_parameters[0]);
    char *endpoint;

    return_value = append_query_params(AUTHORIZE_ENDPOINT_BASE, query_parameters, parameters_len, &endpoint);
    if (return_value != STATUS_SUCCESS)
        goto cleanup;

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
        fprintf(stderr, "Failed to create authorization endpoint\n");
        goto cleanup;
    }

    int max_input_len = 255;
    printf("Open the link to login and accept: %s\n", authorization_endpoint);
    return_value = get_input("After logging in paste \"code\" from the redirect URI query params here\n", max_input_len,
                             &redirect_code);

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
    access_token *token = NULL;

    int return_value = authorize_application(&redirect_code);
    if (return_value != STATUS_SUCCESS) {
        fprintf(stderr, "Failed to authorize application\n");
        goto cleanup;
    }

    return_value = get_access_token_from_authorization_code(redirect_code, &token);
    if (return_value != STATUS_SUCCESS) {
        fprintf(stderr, "Failed to fetch access token\n");
        goto cleanup;
    }

    return_value = write_file_overwrite(REFRESH_TOKEN_FILE_PATH, token->refresh_token);
    if (return_value != STATUS_SUCCESS) {
        fprintf(stderr, "Failed to write refresh token to disk\n");
        goto cleanup;
    }

    return_value = STATUS_SUCCESS;

cleanup:
    free(redirect_code);
    free(token);

    return return_value;
}

int refresh_access_token(access_token **access_token_out) {
    char *refresh_token = NULL;
    access_token *token = NULL;

    int return_value = read_file_content(REFRESH_TOKEN_FILE_PATH, &refresh_token);
    if (return_value != STATUS_SUCCESS) {
        fprintf(stderr, "Failed to read refresh token file. Have you logged in?\n");
        goto cleanup;
    }

    return_value = get_access_token_from_refresh_token(refresh_token, &token);
    if (return_value != STATUS_SUCCESS) {
        fprintf(stderr, "Failed to fetch new access token using refresh token\n");
        goto cleanup;
    }

    *access_token_out = token;
    return_value = STATUS_SUCCESS;

cleanup:
    free(refresh_token);
    return return_value;
}

int spotify_get(char *url, access_token *access_token, char **response_out) {
    struct curl_slist *header = NULL;

    int return_value = append_header("Authorization: Bearer ", access_token->access_token, &header);
    if (return_value != STATUS_SUCCESS) {
        fprintf(stderr, "Failed to create bearer token header\n");
        goto cleanup;
    }

    char *response = NULL;
    return_value = http_request(url, header, NULL, "GET", &response);
    if (return_value != STATUS_SUCCESS) {
        fprintf(stderr, "Request to spotify API failed\n");
        goto cleanup;
    }

    *response_out = response;
    return_value = STATUS_SUCCESS;

cleanup:
    curl_slist_free_all(header);
    return return_value;
}

int parse_items_array(char *input, cJSON **items_out) {
    cJSON *json = NULL;
    int return_value = STATUS_ERROR;

    json = cJSON_Parse(input);
    if (!json) {
        fprintf(stderr, "Response did not contain valid JSON\n");
        goto cleanup;
    }

    cJSON *items = cJSON_DetachItemFromObject(json, "items");
    if (!items || !cJSON_IsArray(items)) {
        fprintf(stderr, "Expected 'items' array\n");
        goto cleanup;
    }

    *items_out = items;
    return_value = STATUS_SUCCESS;

cleanup:
    cJSON_Delete(json);
    return return_value;
}

int parse_artist(cJSON *item, artist *artist) {
    cJSON *name = cJSON_GetObjectItem(item, "name");
    if (!cJSON_IsString(name))
        return STATUS_ERROR;

    snprintf(artist->name, sizeof(artist->name), "%s", name->valuestring);
    return STATUS_SUCCESS;
}

int parse_artists(char *input, artist **artists_out, int *artists_len_out) {
    cJSON *items = NULL;
    artist *artists = NULL;

    int return_value = parse_items_array(input, &items);
    if (return_value != STATUS_SUCCESS) {
        fprintf(stderr, "Input did not contain items array\n");
        goto cleanup;
    }

    int count = cJSON_GetArraySize(items);

    artists = malloc(count * sizeof(artist));
    if (artists == NULL) {
        perror("malloc");
        goto cleanup;
    }

    cJSON *item = NULL;
    int i = 0;
    cJSON_ArrayForEach(item, items) {
        if (parse_artist(item, &artists[i]) != STATUS_SUCCESS)
            goto cleanup;

        i++;
    }

    *artists_out = artists;
    *artists_len_out = count;
    return_value = STATUS_SUCCESS;

cleanup:
    cJSON_Delete(items);
    if (return_value != STATUS_SUCCESS)
        free(artists);

    return return_value;
}

int parse_song(cJSON *item, song *song) {
    cJSON *name = cJSON_GetObjectItem(item, "name");
    cJSON *artists = cJSON_GetObjectItem(item, "artists");

    if (!cJSON_IsString(name) || !cJSON_IsArray(artists))
        return STATUS_ERROR;

    snprintf(song->name, sizeof(song->name), "%s", name->valuestring);

    int artists_len = cJSON_GetArraySize(artists);
    cJSON *artist;
    int i = 0;
    size_t pos = 0;

    cJSON_ArrayForEach(artist, artists) {
        cJSON *artist_name = cJSON_GetObjectItem(artist, "name");

        if (!cJSON_IsString(artist_name))
            return STATUS_ERROR;

        pos += snprintf(song->artist + pos, sizeof(song->artist) - pos, "%s%s", artist_name->valuestring,
                        (i < artists_len - 1) ? ", " : "");

        i++;
    }

    return STATUS_SUCCESS;
}

int parse_songs(char *input, song **songs_out, int *songs_len_out) {
    cJSON *items = NULL;
    song *songs = NULL;

    int return_value = parse_items_array(input, &items);
    if (return_value != STATUS_SUCCESS) {
        fprintf(stderr, "Input did not contain items array\n");
        goto cleanup;
    }

    int count = cJSON_GetArraySize(items);

    songs = malloc(count * sizeof(song));
    if (songs == NULL) {
        perror("malloc");
        goto cleanup;
    }

    cJSON *item = NULL;
    int i = 0;
    cJSON_ArrayForEach(item, items) {
        if (parse_song(item, &songs[i]) != STATUS_SUCCESS)
            goto cleanup;

        i++;
    }

    *songs_out = songs;
    *songs_len_out = count;
    return_value = STATUS_SUCCESS;

cleanup:
    cJSON_Delete(items);
    if (return_value != STATUS_SUCCESS)
        free(songs);

    return return_value;
}

int get_top_artists(access_token *access_token, artist **artists_out, int *artists_len_out) {
    char *response = NULL;

    int return_value = spotify_get(TOP_ARTISTS_ENDPOINT, access_token, &response);
    if (return_value != STATUS_SUCCESS)
        goto cleanup;

    artist *artists = NULL;
    int artists_len;

    return_value = parse_artists(response, &artists, &artists_len);
    if (return_value != STATUS_SUCCESS) {
        fprintf(stderr, "Failed to parse response from spotify API\n");
        goto cleanup;
    }

    *artists_out = artists;
    *artists_len_out = artists_len;
    return_value = STATUS_SUCCESS;

cleanup:
    free(response);
    return return_value;
}

int get_top_songs(access_token *access_token, song **songs_out, int *songs_len_out) {
    char *response = NULL;

    int return_value = spotify_get(TOP_TRACKS_ENDPOINT, access_token, &response);
    if (return_value != STATUS_SUCCESS)
        goto cleanup;

    song *songs = NULL;
    int songs_len;

    return_value = parse_songs(response, &songs, &songs_len);
    if (return_value != STATUS_SUCCESS) {
        fprintf(stderr, "Failed to parse response from spotify API\n");
        goto cleanup;
    }

    *songs_out = songs;
    *songs_len_out = songs_len;
    return_value = STATUS_SUCCESS;

cleanup:
    free(response);
    return return_value;
}

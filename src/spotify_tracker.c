#include "common.h"
#include "network.h"
#include "spotify_api.h"
#include <curl/curl.h>
#include <getopt.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

void print_usage(char *arg) {
    fprintf(stderr, "Usage: %s [-l|--login] [args...]\n", arg);
}

int post_artists_to_gist() {
    access_token *token = NULL;
    artist *artists = NULL;
    song *songs = NULL;

    int return_value = refresh_access_token(&token);
    if (return_value != STATUS_SUCCESS) {
        goto cleanup;
    }

    int artists_len;
    return_value = get_top_artists(token, &artists, &artists_len);
    if (return_value != STATUS_SUCCESS) {
        fprintf(stderr, "Failed to fetch top artists\n");
        goto cleanup;
    }

    for (int i = 0; i < artists_len; i++) {
        printf("%s\n", artists[i].name);
    }

    int songs_len;
    return_value = get_top_songs(token, &songs, &songs_len);
    if (return_value != STATUS_SUCCESS) {
        fprintf(stderr, "Failed to fetch top songs\n");
        goto cleanup;
    }

    for (int i = 0; i < songs_len; i++) {
        printf("%s - %s\n", songs[i].name, songs[i].artist);
    }

    return_value = STATUS_SUCCESS;

cleanup:
    free(token);
    free(artists);
    free(songs);

    return return_value;
}

int main(int argc, char *argv[]) {
    CURLcode res = curl_global_init(CURL_GLOBAL_ALL);

    if (res) {
        return res;
    }

    static struct option long_options[] = {{"login", no_argument, 0, 'l'}, {0, 0, 0, 0}};
    int opt;
    int return_value = STATUS_ERROR;
    bool is_logging_in = false;

    while ((opt = getopt_long(argc, argv, "l", long_options, NULL)) != -1) {
        switch (opt) {
        case 'l':
            is_logging_in = true;
            break;
        }
    }

    if (is_logging_in) {
        return_value = login();
    } else {
        return_value = post_artists_to_gist();
    }

    curl_global_cleanup();
    return return_value;
}

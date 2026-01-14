#ifndef SPOTIFY_API_H
#define SPOTIFY_API_H

#include "common.h"
#include "network.h"

typedef struct {
    char name[512];
} artist;

typedef struct {
    char name[512];
    char artist[512];
} song;

status_code login();
status_code refresh_access_token(access_token **access_token_out);
status_code get_top_artists(const access_token *access_token, artist **artists_out, int *artists_len_out);
status_code get_top_songs(const access_token *access_token, song **songs_out, int *songs_len_out);

#endif

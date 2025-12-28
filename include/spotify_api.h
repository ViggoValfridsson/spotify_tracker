#ifndef SPOTIFY_API_H
#define SPOTIFY_API_H

#include "network.h"

typedef struct {

} artist;

typedef struct {

} song;

int login();
int refresh_access_token(access_token **access_token_out);
int get_top_artists(access_token *access_token, artist **artists_out);
int get_top_songs(access_token *access_token, artist **songs_out);

#endif

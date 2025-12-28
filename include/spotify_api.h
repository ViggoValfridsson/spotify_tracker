#ifndef SPOTIFY_API_H
#define SPOTIFY_API_H

typedef struct {

} artist;

typedef struct {

} song;

int login();
int get_top_artists(artist **artists_out);
int get_top_songs(artist **songs_out);

#endif

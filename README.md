# Work in progress
 
This project is not done and no functionality is guaranteed to work. This README is currently in an unfinished state.

# Spotify Tracker

A simple C program that fetches your top artists and songs and writes them to a github gist.

## Dependencies

Make sure you have libcurl installed

## Running the program

1. Setup a spotify app and add "https://httpbin.org/anything" to allowed redirect URIs. https://developer.spotify.com/documentation/web-api/concepts/apps
2. Create credentials file at `~/.config/spotify-tracker/spotify-credentials`. (This will be replaced by option to automatically create this file)
3. Add this to credentials file

```json
{
    "clientId": "<client-id>",
    "clientSecret": "<client-secret>"
}
```
4. Create the a github gist to be used for uploading spotify data
5. Generate a fine-grained PAT with read/write for gists (it is not possible to scope a PAT to a specific gist)
6. Create this file `~/.config/spotify-tracker/github-config` with this content

```json
{
    "gistId": "<gist-id>",
    "fileName": "<file-name>",
    "pat": "<pat>"
}
```

# Work in progress
 
This project is not done and no functionality is guaranteed to work.

# Spotify Tracker

A simple C program that fetches your top artists and songs and writes them to a github gist.

## Running the program

1. Setup a spotify app (you can set redirect URIs to anything, we will not use it). https://developer.spotify.com/documentation/web-api/concepts/apps
2. Create credentials file at `~/.config/spotify-tracker/credentials`. (This will be replaced by option to automatically create this file)
3. Add this to credentials file

```json
{
    "clientId": "<client-id>",
    "clientSecret": "<client-secret>"
}
```

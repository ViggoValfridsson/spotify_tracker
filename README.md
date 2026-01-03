# Work in progress
 
This project is not done and no functionality is guaranteed to work. This README is currently in an unfinished state.

# Spotify Tracker

A simple C program that fetches your top artists and songs and writes them to a github repository.

## Dependencies

Make sure you have libcurl installed

## Running the program

1. Setup a spotify app and add "https://httpbin.org/anything" to allowed redirect URIs. 
https://developer.spotify.com/documentation/web-api/concepts/apps
2. Create credentials file at `~/.config/spotify-tracker/spotify-credentials`. (This will be replaced by option to 
automatically create this file)
3. Add this to credentials file

```json
{
    "clientId": "<client-id>",
    "clientSecret": "<client-secret>"
}
```
4. Create the a github repository to be used for uploading spotify data. The application is intended to write to your 
profiles README.md but will work with any file
5. Generate a fine-grained PAT with contents Read and write for the repository
6. Create this file `~/.config/spotify-tracker/github-config` with this content. Note the file will be created or overwritten.

```json
{
    "repoOwner": "<owner-name>",
    "repoName": "<repo-name>",
    "fileName": "<file-name>",
    "committer": "<commiter>",
    "email": "<email>",
    "pat": "<pat>"
}
```

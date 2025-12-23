#include "common.h"
#include "spotify_api.h"
#include <curl/curl.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>

void print_usage(char *arg) {
    fprintf(stderr, "Usage: %s [-l|--login] [args...]\n", arg);
}

int main(int argc, char *argv[]) {
    CURLcode res = curl_global_init(CURL_GLOBAL_ALL);

    if (res) {
        return res;
    }

    static struct option long_options[] = {{"login", no_argument, 0, 'l'}, {0, 0, 0, 0}};
    int opt;

    while ((opt = getopt_long(argc, argv, "l", long_options, NULL)) != -1) {
        switch (opt) {
        case 'l':
            return login();
            break;
        case '?':
        default:
            print_usage(argv[0]);
            return STATUS_ERROR;
        }
    }

    curl_global_cleanup();
}

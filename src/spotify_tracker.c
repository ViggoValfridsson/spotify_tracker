#include "spotify_api.h"
#include <curl/curl.h>

int main(int argc, char *argv[]) {
    CURLcode res = curl_global_init(CURL_GLOBAL_ALL);

    if (res) {
        return res;
    }

    int tmp = get_top_artists();
    curl_global_cleanup();
}

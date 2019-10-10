// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <stdio.h>
#include <curl/curl.h>

int main(char **argv)
{
    CURL *curl;
    curl = curl_easy_init();
    int result;

    FILE *f;
    f = fopen("here.jpg", "wb");

    printf("\n1...");
    
    curl_easy_setopt(curl, CURLOPT_URL, "http://www.laboratoons.com/wp-content/uploads/2018/11/Alternativas-A.jpg");
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, f);
    curl_easy_setopt(curl, CURLOPT_FAILONERROR, 1l);

    result = curl_easy_perform(curl);

    if (result == CURLE_OK) {
        printf("\n Done.");
    } else {
        printf("\n Error: %s\n", curl_easy_strerror(result));
    }

    // clean up
    fclose(f);
    curl_easy_cleanup(curl);

    printf("Hello world");
    return 0;
}

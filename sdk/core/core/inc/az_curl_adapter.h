// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#ifndef AZ_CURL_ADAPTER
#define AZ_CURL_ADAPTER

#include <stdio.h>
#include <curl/curl.h>
#include <az_span.h>

CURL *curl;

az_result az_http_client_init_impl() {
    curl = curl_easy_init();
    curl_easy_setopt(curl, CURLOPT_FAILONERROR, 1l);
    return AZ_OK;
}

az_result az_http_client_set_URL_impl(az_const_span span) {
    curl_easy_setopt(curl, CURLOPT_URL, span.begin);
}

az_result az_http_client_download_to_file_impl(az_const_span path) {
    FILE *f;
    f = fopen(path.begin, "wb");
    int result;

    curl_easy_setopt(curl, CURLOPT_WRITEDATA, f);
    result = curl_easy_perform(curl);
    fclose(f);

    if (result == CURLE_OK) {
        return AZ_OK;
    } else {
        return AZ_ERROR_HTTP_CLIENT_ERROR;
    }
}

az_result az_http_client_clean_impl() {
    curl_easy_cleanup(curl);
    return AZ_OK;
}

#endif
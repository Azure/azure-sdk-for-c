// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#ifndef AZ_HTTP_CLIENT
#define AZ_HTTP_CLIENT

#ifdef MOCK_CURL
#include <az_mock_curl.h>
#else
#include <az_curl_adapter.h>
#endif

#include <az_span.h>

az_result az_http_client_init() {
    return az_http_client_init_impl();
}

az_result az_http_client_set_URL(az_const_span span) {
    return az_http_client_set_URL_impl(span);
}

az_result az_http_client_download_to_file(az_const_span path) {
    return az_http_client_download_to_file_impl(path);
}

az_result az_http_client_clean() {
    return az_http_client_clean_impl();
}

#endif
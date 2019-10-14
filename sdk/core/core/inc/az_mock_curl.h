// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#ifndef AZ_MOCK_CURL
#define AZ_MOCK_CURL

#include <az_span.h>
#include <stdio.h>

az_result az_http_client_init_impl() {
    printf("\nHello from mock. Init called.");
    return AZ_OK;
}

az_result az_http_client_set_URL_impl(az_const_span span) {
    printf("\nSet URL called");
    return AZ_OK;
}

az_result az_http_client_download_to_file_impl(az_const_span path) {
    printf("\nDownload to file called %s", path.begin);
    return AZ_OK;
}

az_result az_http_client_clean_impl() {
    printf("\nClean UP");
    return AZ_OK;
}

#endif
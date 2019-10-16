// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#ifndef AZ_CURL_ADAPTER
#define AZ_CURL_ADAPTER

#include <az_http_request.h>
#include <az_span.h>
#include <curl/curl.h>
#include <stdio.h>

CURL * curl;
CURLcode res;
struct curl_slist * chunk = NULL;

az_result az_http_client_init_impl();
az_result az_http_client_set_URL_impl(az_const_span span);
az_result az_http_client_download_to_file_impl(az_const_span path);
az_result az_http_client_clean_impl();
az_result az_http_client_send_impl(az_http_request const * const request);

#endif
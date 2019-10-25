// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <az_http_request_builder.h>

#include <_az_cfg.h>

az_result az_http_request_builder_init(
    az_http_request_builder * const p_hrb,
    az_span const buffer,
    int16_t const max_url_size,
    az_const_span const method_verb,
    az_const_span const initial_url) {
  (void)p_hrb;
  (void)buffer;
  (void)max_url_size;
  (void)method_verb;
  (void)initial_url;
  return AZ_OK;
}

az_result az_http_request_builder_set_query_parameter(
    az_http_request_builder * const p_hrb,
    az_const_span const name,
    az_const_span const value) {
  (void)p_hrb;
  (void)name;
  (void)value;
  return AZ_OK;
}

az_result az_http_request_builder_append_header(
    az_http_request_builder * const p_hrb,
    az_const_span const name,
    az_const_span const value) {
  (void)p_hrb;
  (void)name;
  (void)value;
  return AZ_OK;
}

az_result az_http_request_builder_mark_retry_headers_start(az_http_request_builder * const p_hrb) {
  (void)p_hrb;
  return AZ_OK;
}

az_result az_http_request_builder_remove_retry_headers(az_http_request_builder * const p_hrb) {
  (void)p_hrb;
  return AZ_OK;
}

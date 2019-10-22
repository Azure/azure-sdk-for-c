// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <az_http_request_builder.h>

az_result az_http_request_builder_init(
    az_http_request_builder * const out,
    az_span const buffer,
    az_const_span const method_verb) {
  return AZ_OK;
}

az_result az_http_request_builder_set_initial_url(
    az_http_request_builder * const p_builder,
    az_const_span const url) {
  return AZ_OK;
}

az_result az_http_request_builder_set_query_parameter(
    az_http_request_builder * const p_builder,
    az_const_span const name,
    az_const_span const value) {
  return AZ_OK;
}

az_result az_http_request_builder_append_header(
    az_http_request_builder * const p_builder,
    az_const_span const name,
    az_const_span const value) {
  return AZ_OK;
}

az_result az_http_request_builder_mark_retry_headers_start(
    az_http_request_builder * const p_builder) {
  return AZ_OK;
}

az_result az_http_request_builder_remove_retry_headers(az_http_request_builder * const p_builder) {
  return AZ_OK;
}
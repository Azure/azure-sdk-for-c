// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

/**
 * @file az_http_request_builder.c
 * @author https://github.com/Azure/azure-sdk-for-c
 * @brief Interface implementation for bulding an HTTP Request. Bulder maintans the state of a
 * control block for writing request into a buffer
 * @date 2019-10-23
 *
 */

#include <az_http_request_builder.h>

/**
 * @brief
 *
 * @param p_hrb
 * @param buffer
 * @param max_url_size
 * @param method_verb
 * @param initial_url
 * @return az_result
 */
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

/**
 * @brief
 *
 * @param p_hrb
 * @param name
 * @param value
 * @return az_result
 */
az_result az_http_request_builder_set_query_parameter(
    az_http_request_builder * const p_hrb,
    az_const_span const name,
    az_const_span const value) {
  (void)p_hrb;
  (void)name;
  (void)value;
  return AZ_OK;
}

/**
 * @brief
 *
 * @param p_hrb
 * @param name
 * @param value
 * @return az_result
 */
az_result az_http_request_builder_append_header(
    az_http_request_builder * const p_hrb,
    az_const_span const name,
    az_const_span const value) {
  (void)p_hrb;
  (void)name;
  (void)value;
  return AZ_OK;
}

/**
 * @brief
 *
 * @param p_hrb
 * @return az_result
 */
az_result az_http_request_builder_mark_retry_headers_start(az_http_request_builder * const p_hrb) {
  (void)p_hrb;
  return AZ_OK;
}

/**
 * @brief
 *
 * @param p_hrb
 * @return az_result
 */
az_result az_http_request_builder_remove_retry_headers(az_http_request_builder * const p_hrb) {
  (void)p_hrb;
  return AZ_OK;
}

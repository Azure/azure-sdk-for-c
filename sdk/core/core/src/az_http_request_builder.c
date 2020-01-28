// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include "az_span_private.h"
#include "az_str_private.h"
#include <az_contract_internal.h>
#include <az_http.h>
#include <az_span_internal.h>

#include <assert.h>

#include <_az_cfg.h>

az_span const AZ_HTTP_METHOD_VERB_GET = AZ_SPAN_LITERAL_FROM_STR("GET");
az_span const AZ_HTTP_METHOD_VERB_HEAD = AZ_SPAN_LITERAL_FROM_STR("HEAD");
az_span const AZ_HTTP_METHOD_VERB_POST = AZ_SPAN_LITERAL_FROM_STR("POST");
az_span const AZ_HTTP_METHOD_VERB_PUT = AZ_SPAN_LITERAL_FROM_STR("PUT");
az_span const AZ_HTTP_METHOD_VERB_DELETE = AZ_SPAN_LITERAL_FROM_STR("DELETE");
az_span const AZ_HTTP_METHOD_VERB_TRACE = AZ_SPAN_LITERAL_FROM_STR("TRACE");
az_span const AZ_HTTP_METHOD_VERB_OPTIONS = AZ_SPAN_LITERAL_FROM_STR("OPTIONS");
az_span const AZ_HTTP_METHOD_VERB_CONNECT = AZ_SPAN_LITERAL_FROM_STR("CONNECT");
az_span const AZ_HTTP_METHOD_VERB_PATCH = AZ_SPAN_LITERAL_FROM_STR("PATCH");

AZ_INLINE az_pair * get_headers_start(az_span buffer, int32_t max_url_size) {
  // 8-byte address alignment
  return (az_pair *)((uintptr_t)(az_span_ptr(buffer) + max_url_size + 7) & ~(uintptr_t)7);
}

AZ_INLINE int32_t get_headers_max(az_span const buffer, az_pair * const headers_start) {
  // We need to compare both pointers as pointers to the same type (uint8_t*)
  uint8_t * const buffer_end = az_span_ptr(buffer) + az_span_length(buffer);
  uint8_t * const headers_start_uint8ptr = (uint8_t *)headers_start;

  int32_t const max_headers = (buffer_end - headers_start_uint8ptr) / sizeof(az_pair);

  // If there's enough space in the buffer, return the max number of elements it can fit (capped to
  // uint16_t's max)
  return buffer_end > headers_start_uint8ptr ? (max_headers > ~0 ? ~0 : max_headers) : 0;
}

AZ_NODISCARD az_result az_http_request_builder_init(
    az_http_request_builder * p_hrb,
    az_span buffer,
    int32_t max_url_size,
    az_span method_verb,
    az_span initial_url,
    az_span body) {
  AZ_CONTRACT_ARG_NOT_NULL(p_hrb);
  AZ_CONTRACT_ARG_VALID_SPAN(buffer);
  AZ_CONTRACT_ARG_VALID_SPAN(method_verb);
  AZ_CONTRACT_ARG_VALID_SPAN(initial_url);

  if (max_url_size < az_span_length(initial_url)) {
    return AZ_ERROR_ARG;
  }

  if (az_span_capacity(buffer) < max_url_size) {
    return AZ_ERROR_BUFFER_OVERFLOW;
  }

  // Create url builder
  az_span url_builder = az_span_take(buffer, max_url_size);
  // write initial url
  AZ_RETURN_IF_FAILED(az_span_append(url_builder, initial_url, &url_builder));

  az_pair * const headers_start = get_headers_start(buffer, max_url_size);
  uint16_t const max_headers = get_headers_max(buffer, headers_start);

  *p_hrb = (az_http_request_builder){
    .buffer = buffer,
    .method_verb = method_verb,
    .url_builder = url_builder,
    .max_headers = max_headers,
    .retry_headers_start = max_headers,
    .headers_end = 0,
    .body = body,
    .query_start = 0,
  };

  return AZ_OK;
}

AZ_NODISCARD az_result az_http_request_builder_set_query_parameter(
    az_http_request_builder * p_hrb,
    az_span name,
    az_span value) {
  AZ_CONTRACT_ARG_NOT_NULL(p_hrb);
  AZ_CONTRACT_ARG_VALID_SPAN(name);
  AZ_CONTRACT_ARG_VALID_SPAN(value);

  // name or value can't be empty
  if (az_span_length(name) == 0 || az_span_length(value) == 0) {
    return AZ_ERROR_ARG;
  }

  int32_t new_url_size;
  {
    int32_t const extra_chars_size = AZ_STRING_LITERAL_LEN("?=");
    int32_t const name_length = az_span_length(name);
    int32_t const value_length = az_span_length(value);
    int32_t const name_and_value_size = name_length + value_length;
    int32_t const appended_size = name_and_value_size + extra_chars_size;

    int32_t const url_builder_length = az_span_length(p_hrb->url_builder);
    new_url_size = url_builder_length + appended_size;

    // check for integer overflows, and finally whether the result would fit
    if (name_and_value_size < name_length || name_and_value_size < value_length
        || appended_size < name_and_value_size || appended_size < extra_chars_size
        || new_url_size < appended_size || new_url_size < url_builder_length
        || new_url_size > az_span_capacity(p_hrb->url_builder)) {
      return AZ_ERROR_BUFFER_OVERFLOW;
    }
  }

  // Append either '?' or '&'
  AZ_RETURN_IF_FAILED(az_span_append(
      p_hrb->url_builder,
      p_hrb->query_start != 0 ? AZ_SPAN_FROM_STR("&") : AZ_SPAN_FROM_STR("?"),
      &p_hrb->url_builder));
  // update QPs starting position when it's 0
  if (p_hrb->query_start == 0) {
    p_hrb->query_start = az_span_length(p_hrb->url_builder);
  }

  // Append parameter name
  AZ_RETURN_IF_FAILED(az_span_append(p_hrb->url_builder, name, &p_hrb->url_builder));

  // Append equal sym
  AZ_RETURN_IF_FAILED(
      az_span_append(p_hrb->url_builder, AZ_SPAN_FROM_STR("="), &p_hrb->url_builder));

  // Parameter value
  AZ_RETURN_IF_FAILED(az_span_append(p_hrb->url_builder, value, &p_hrb->url_builder));

  assert(az_span_length(p_hrb->url_builder) == new_url_size);

  return AZ_OK;
}

AZ_NODISCARD az_result
az_http_request_builder_append_header(az_http_request_builder * p_hrb, az_span key, az_span value) {
  AZ_CONTRACT_ARG_NOT_NULL(p_hrb);
  AZ_CONTRACT_ARG_VALID_SPAN(key);
  AZ_CONTRACT_ARG_VALID_SPAN(value);

  AZ_CONTRACT(az_span_length(key) == 0 || az_span_length(value) == 0, AZ_ERROR_ARG);

  if (p_hrb->headers_end >= p_hrb->max_headers) {
    return AZ_ERROR_BUFFER_OVERFLOW;
  }

  az_pair * const headers = get_headers_start(p_hrb->buffer, az_span_capacity(p_hrb->url_builder));
  headers[p_hrb->headers_end] = (az_pair){ .key = key, .value = value };
  p_hrb->headers_end += 1;

  return AZ_OK;
}

AZ_NODISCARD az_result
az_http_request_builder_append_path(az_http_request_builder * p_hrb, az_span path) {
  AZ_CONTRACT_ARG_NOT_NULL(p_hrb);
  AZ_CONTRACT_ARG_NOT_NULL(&path);

  // check if there is enough space yet
  int32_t const current_ulr_size = az_span_length(p_hrb->url_builder);
  int32_t const url_after_path_size
      = current_ulr_size + az_span_length(path) + 1 /* separator '/' to be added */;
  int32_t query_start
      = p_hrb->query_start != 0 ? p_hrb->query_start - 1 : az_span_length(p_hrb->url_builder);

  if (url_after_path_size >= az_span_capacity(p_hrb->url_builder)) {
    return AZ_ERROR_BUFFER_OVERFLOW;
  }

  // Use replace twice. Yes, we will have 2 right shift (one on each replace), but we rely on
  // replace function for doing this movements only and avoid updating manually. We could also
  // create a temp buffer to join "/" and path and then use replace. But that will cost us more
  // stack memory.
  AZ_RETURN_IF_FAILED(
      az_span_replace(&p_hrb->url_builder, query_start, query_start, AZ_SPAN_FROM_STR("/")));
  query_start += 1; // a size of "/"
  AZ_RETURN_IF_FAILED(az_span_replace(&p_hrb->url_builder, query_start, query_start, path));
  query_start += az_span_length(path);

  // update query start
  if (p_hrb->query_start) {
    p_hrb->query_start = query_start + 1;
  }

  return AZ_OK;
}

AZ_NODISCARD az_result
az_http_request_builder_mark_retry_headers_start(az_http_request_builder * p_hrb) {
  AZ_CONTRACT_ARG_NOT_NULL(p_hrb);
  p_hrb->retry_headers_start = p_hrb->headers_end;
  return AZ_OK;
}

AZ_NODISCARD az_result
az_http_request_builder_remove_retry_headers(az_http_request_builder * p_hrb) {
  AZ_CONTRACT_ARG_NOT_NULL(p_hrb);

  if (p_hrb->headers_end > p_hrb->retry_headers_start) {
    p_hrb->headers_end = p_hrb->retry_headers_start;
  }

  return AZ_OK;
}

AZ_NODISCARD az_result az_http_request_builder_get_header(
    az_http_request_builder * p_hrb,
    uint16_t index,
    az_pair * out_result) {
  AZ_CONTRACT_ARG_NOT_NULL(p_hrb);
  AZ_CONTRACT_ARG_NOT_NULL(out_result);

  if (index >= p_hrb->headers_end) {
    return AZ_ERROR_ARG;
  }

  az_pair * const headers = get_headers_start(p_hrb->buffer, az_span_capacity(p_hrb->url_builder));
  *out_result = headers[index];
  return AZ_OK;
}

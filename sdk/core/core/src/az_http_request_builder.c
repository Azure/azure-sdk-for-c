// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <az_http_request_builder.h>

#include "../inc/internal/_az_mut_span.h"
#include "../inc/internal/_az_span.h"
#include "../inc/internal/az_contract.h"
#include <az_str.h>

#include <assert.h>

#include <_az_cfg.h>

#define AZ_CONTRACT_ARG_VALID_MUT_SPAN(span) AZ_CONTRACT(az_mut_span_is_valid(span), AZ_ERROR_ARG)
#define AZ_CONTRACT_ARG_VALID_SPAN(span) AZ_CONTRACT(az_span_is_valid(span), AZ_ERROR_ARG)

az_span const AZ_HTTP_METHOD_VERB_GET = AZ_CONST_STR("GET");
az_span const AZ_HTTP_METHOD_VERB_HEAD = AZ_CONST_STR("HEAD");
az_span const AZ_HTTP_METHOD_VERB_POST = AZ_CONST_STR("POST");
az_span const AZ_HTTP_METHOD_VERB_PUT = AZ_CONST_STR("PUT");
az_span const AZ_HTTP_METHOD_VERB_DELETE = AZ_CONST_STR("DELETE");
az_span const AZ_HTTP_METHOD_VERB_TRACE = AZ_CONST_STR("TRACE");
az_span const AZ_HTTP_METHOD_VERB_OPTIONS = AZ_CONST_STR("OPTIONS");
az_span const AZ_HTTP_METHOD_VERB_CONNECT = AZ_CONST_STR("CONNECT");
az_span const AZ_HTTP_METHOD_VERB_PATCH = AZ_CONST_STR("PATCH");

AZ_INLINE az_pair * get_headers_start(az_mut_span const buffer, int16_t const max_url_size) {
  // 8-byte address alignment
  return (az_pair *)((uintptr_t)(buffer.begin + max_url_size + 7) & ~(uintptr_t)7);
}

AZ_INLINE uint16_t get_headers_max(az_mut_span const buffer, az_pair * const headers_start) {
  // We need to compare both pointers as pointers to the same type (uint8_t*)
  uint8_t * const buffer_end = buffer.begin + buffer.size;
  uint8_t * const headers_start_uint8ptr = (uint8_t *)headers_start;

  size_t const max_headers = (buffer_end - headers_start_uint8ptr) / sizeof(az_pair);

  // If there's enough space in the buffer, return the max number of elements it can fit (capped to
  // uint16_t's max)
  return buffer_end > headers_start_uint8ptr
      ? (max_headers > (size_t) ~(uint16_t)0 ? ~(uint16_t)0 : (uint16_t)max_headers)
      : 0;
}

AZ_NODISCARD az_result az_http_request_builder_init(
    az_http_request_builder * const p_hrb,
    az_mut_span const buffer,
    uint16_t const max_url_size,
    az_span const method_verb,
    az_span const initial_url,
    az_span const body) {
  AZ_CONTRACT_ARG_NOT_NULL(p_hrb);
  AZ_CONTRACT_ARG_VALID_MUT_SPAN(buffer);
  AZ_CONTRACT_ARG_VALID_SPAN(method_verb);
  AZ_CONTRACT_ARG_VALID_SPAN(initial_url);

  if (max_url_size < initial_url.size) {
    return AZ_ERROR_ARG;
  }

  if (buffer.size < max_url_size) {
    return AZ_ERROR_BUFFER_OVERFLOW;
  }

  // Create url builder
  az_span_builder url_builder = az_span_builder_create(az_mut_span_take(buffer, max_url_size));
  // write initial url
  AZ_RETURN_IF_FAILED(az_span_builder_append(&url_builder, initial_url));

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
    az_http_request_builder * const p_hrb,
    az_span const name,
    az_span const value) {
  AZ_CONTRACT_ARG_NOT_NULL(p_hrb);
  AZ_CONTRACT_ARG_VALID_SPAN(name);
  AZ_CONTRACT_ARG_VALID_SPAN(value);

  // name or value can't be empty
  if (name.size == 0 || value.size == 0) {
    return AZ_ERROR_ARG;
  }

  size_t new_url_size;
  {
    size_t const extra_chars_size = AZ_STRING_LITERAL_LEN("?=");
    size_t const name_and_value_size = name.size + value.size;
    size_t const appended_size = name_and_value_size + extra_chars_size;

    new_url_size = p_hrb->url_builder.length + appended_size;

    // check for integer overflows, and finally whether the result would fit
    if (name_and_value_size < name.size || name_and_value_size < value.size
        || appended_size < name_and_value_size || appended_size < extra_chars_size
        || new_url_size < appended_size || new_url_size < p_hrb->url_builder.length
        || new_url_size > p_hrb->url_builder.buffer.size) {
      return AZ_ERROR_BUFFER_OVERFLOW;
    }

    az_mut_span const new_url_span
        = { .begin = p_hrb->url_builder.buffer.begin, .size = new_url_size };

    // check whether name or value regions overlap with destination for some reason (unlikely)
    if (az_span_is_overlap(az_mut_span_to_span(new_url_span), name)
        || az_span_is_overlap(az_mut_span_to_span(new_url_span), value)) {
      return AZ_ERROR_ARG;
    }
  }

  // Append either '?' or '&'
  AZ_RETURN_IF_FAILED(
      az_span_builder_append_byte(&p_hrb->url_builder, p_hrb->query_start ? '&' : '?'));
  // update QPs starting position when it's 0
  if (!p_hrb->query_start) {
    p_hrb->query_start = (uint16_t)p_hrb->url_builder.length;
  }

  // Append parameter name
  AZ_RETURN_IF_FAILED(az_span_builder_append(&p_hrb->url_builder, name));

  // Append equal sym
  AZ_RETURN_IF_FAILED(az_span_builder_append_byte(&p_hrb->url_builder, '='));

  // Parameter value
  AZ_RETURN_IF_FAILED(az_span_builder_append(&p_hrb->url_builder, value));

  assert(p_hrb->url_builder.length == new_url_size);

  return AZ_OK;
}

AZ_NODISCARD az_result az_http_request_builder_append_header(
    az_http_request_builder * const p_hrb,
    az_span const key,
    az_span const value) {
  AZ_CONTRACT_ARG_NOT_NULL(p_hrb);
  AZ_CONTRACT_ARG_VALID_SPAN(key);
  AZ_CONTRACT_ARG_VALID_SPAN(value);

  if (key.size == 0 || value.size == 0) {
    return AZ_ERROR_ARG;
  }

  if (p_hrb->headers_end >= p_hrb->max_headers) {
    return AZ_ERROR_BUFFER_OVERFLOW;
  }

  az_pair * const headers
      = get_headers_start(p_hrb->buffer, (int16_t)p_hrb->url_builder.buffer.size);
  headers[p_hrb->headers_end] = (az_pair){ .key = key, .value = value };
  p_hrb->headers_end += 1;

  return AZ_OK;
}

AZ_NODISCARD az_result
az_http_request_builder_append_path(az_http_request_builder * const p_hrb, az_span const path) {
  AZ_CONTRACT_ARG_NOT_NULL(p_hrb);
  AZ_CONTRACT_ARG_NOT_NULL(&path);

  // check if there is enough space yet
  size_t const current_ulr_size = p_hrb->url_builder.length;
  uint16_t const url_after_path_size
      = (uint16_t)current_ulr_size + (uint16_t)path.size + 1 /* separator '/' to be added */;
  uint16_t query_start
      = p_hrb->query_start ? p_hrb->query_start - 1 : (uint16_t)p_hrb->url_builder.length;

  if (url_after_path_size >= p_hrb->url_builder.buffer.size) {
    return AZ_ERROR_BUFFER_OVERFLOW;
  }

  /* use replace twice. Yes, we will have 2 right shift (one on each replace), but we rely on
   * replace functionfor doing this movements only and avoid updating manually. We could also create
   * a temp buffer to join "/" and path and then use replace. But that will cost us more stack
   * memory.
   */
  AZ_RETURN_IF_FAILED(
      az_span_builder_replace(&p_hrb->url_builder, query_start, query_start, AZ_STR("/")));
  query_start += 1; // a size of "/"
  AZ_RETURN_IF_FAILED(az_span_builder_replace(&p_hrb->url_builder, query_start, query_start, path));
  query_start += (uint16_t)path.size;

  // update query start
  if (p_hrb->query_start) {
    p_hrb->query_start = query_start + 1;
  }

  return AZ_OK;
}

AZ_NODISCARD az_result
az_http_request_builder_mark_retry_headers_start(az_http_request_builder * const p_hrb) {
  AZ_CONTRACT_ARG_NOT_NULL(p_hrb);
  p_hrb->retry_headers_start = p_hrb->headers_end;
  return AZ_OK;
}

AZ_NODISCARD az_result
az_http_request_builder_remove_retry_headers(az_http_request_builder * const p_hrb) {
  AZ_CONTRACT_ARG_NOT_NULL(p_hrb);

  if (p_hrb->headers_end > p_hrb->retry_headers_start) {
    p_hrb->headers_end = p_hrb->retry_headers_start;
  }

  return AZ_OK;
}

AZ_NODISCARD az_result az_http_request_builder_get_header(
    az_http_request_builder const * const p_hrb,
    uint16_t const index,
    az_pair * const out_result) {
  AZ_CONTRACT_ARG_NOT_NULL(p_hrb);
  AZ_CONTRACT_ARG_NOT_NULL(out_result);

  if (index >= p_hrb->headers_end) {
    return AZ_ERROR_ARG;
  }

  az_pair * const headers
      = get_headers_start(p_hrb->buffer, (int16_t)p_hrb->url_builder.buffer.size);
  *out_result = headers[index];
  return AZ_OK;
}

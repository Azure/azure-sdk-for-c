// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <az_http_request_builder.h>

#include <az_contract.h>
#include <az_str.h>

#include <assert.h>

#include <_az_cfg.h>

az_const_span const AZ_HTTP_METHOD_VERB_GET = AZ_CONST_STR("GET");
az_const_span const AZ_HTTP_METHOD_VERB_HEAD = AZ_CONST_STR("HEAD");
az_const_span const AZ_HTTP_METHOD_VERB_POST = AZ_CONST_STR("POST");
az_const_span const AZ_HTTP_METHOD_VERB_PUT = AZ_CONST_STR("PUT");
az_const_span const AZ_HTTP_METHOD_VERB_DELETE = AZ_CONST_STR("DELETE");
az_const_span const AZ_HTTP_METHOD_VERB_TRACE = AZ_CONST_STR("TRACE");
az_const_span const AZ_HTTP_METHOD_VERB_OPTIONS = AZ_CONST_STR("OPTIONS");
az_const_span const AZ_HTTP_METHOD_VERB_CONNECT = AZ_CONST_STR("CONNECT");
az_const_span const AZ_HTTP_METHOD_VERB_PATCH = AZ_CONST_STR("PATCH");

AZ_INLINE az_pair * get_headers_start(
    az_span const buffer,
    int16_t const max_url_size,
    int16_t const max_headers) {
  uint8_t * const buffer_begin = buffer.begin;
  uint8_t * const buffer_end = buffer.begin + buffer.size;

  uint8_t * const headers_start
      = (uint8_t *)((uintptr_t)(buffer_begin + max_url_size + 7) & ~(uintptr_t)7);

  uint8_t * const headers_end = (uint8_t *)((az_pair *)headers_start + max_headers);

  return headers_end <= buffer_end && buffer_begin < buffer_end && headers_start < headers_end
      ? (az_pair *)headers_start
      : NULL;
}

AZ_NODISCARD az_result az_http_request_builder_init(
    az_http_request_builder * const p_hrb,
    az_span const buffer,
    az_const_span const method_verb,
    az_const_span const initial_url,
    uint16_t const max_url_size,
    uint16_t const max_headers) {
  AZ_CONTRACT_ARG_NOT_NULL(p_hrb);

  if (!az_span_is_valid(buffer) || !az_const_span_is_valid(method_verb)
      || !az_const_span_is_valid(initial_url) || max_url_size < initial_url.size) {
    return AZ_ERROR_ARG;
  }

  if (buffer.size < max_url_size
      || (max_headers > 0 && get_headers_start(buffer, max_url_size, max_headers) == NULL)) {
    return AZ_ERROR_BUFFER_OVERFLOW;
  }

  AZ_RETURN_IF_FAILED(az_span_set(buffer, '\0'));

  az_span uri_buf = { 0, 0 };
  AZ_RETURN_IF_FAILED(az_span_copy(
      (az_span){ .begin = buffer.begin, .size = max_url_size }, initial_url, &uri_buf));

  *p_hrb = (az_http_request_builder){ .buffer = buffer,
                                      .method_verb = method_verb,
                                      .url = uri_buf,
                                      .max_url_size = max_url_size,
                                      .max_headers = max_headers,
                                      .retry_headers_start = max_headers,
                                      .headers_end = 0 };

  return AZ_OK;
}

AZ_NODISCARD az_result az_http_request_builder_set_query_parameter(
    az_http_request_builder * const p_hrb,
    az_const_span const name,
    az_const_span const value) {
  AZ_CONTRACT_ARG_NOT_NULL(p_hrb);

  if (!az_const_span_is_valid(name) || !az_const_span_is_valid(value) || name.size == 0) {
    return AZ_ERROR_ARG;
  }

  size_t new_url_size;
  bool const has_value = value.size > 0;
  {
    az_span new_url_span = { .begin = p_hrb->url.begin };
    size_t const extra_chars_size = sizeof((uint8_t)'?') + (has_value ? sizeof((uint8_t)'=') : 0);
    size_t const name_and_value_size = name.size + value.size;
    size_t const appended_size = name_and_value_size + extra_chars_size;

    new_url_size = p_hrb->url.size + appended_size;

    if (name_and_value_size < name.size || name_and_value_size < value.size
        || appended_size < name_and_value_size || appended_size < extra_chars_size
        || new_url_size < appended_size || new_url_size < p_hrb->url.size
        || new_url_size > p_hrb->max_url_size) {
      return AZ_ERROR_BUFFER_OVERFLOW;
    }

    new_url_span.size = new_url_size;

    if (az_const_span_is_overlap(az_span_to_const_span(new_url_span), name)
        || az_const_span_is_overlap(az_span_to_const_span(new_url_span), value)) {
      return AZ_ERROR_ARG;
    }
  }

  bool first_parameter = true;
  for (size_t i = p_hrb->url.size; i > 0;) {
    --i;
    if (p_hrb->url.begin[i] == '?') {
      first_parameter = false;
    }
  }

  p_hrb->url.begin[p_hrb->url.size] = first_parameter ? '?' : '&';
  ++(p_hrb->url.size);

  memcpy(p_hrb->url.begin + p_hrb->url.size, name.begin, name.size);
  p_hrb->url.size += name.size;

  if (has_value) {
    p_hrb->url.begin[p_hrb->url.size] = '=';
    ++(p_hrb->url.size);

    memcpy(p_hrb->url.begin + p_hrb->url.size, value.begin, value.size);
    p_hrb->url.size += value.size;
  }

  assert(p_hrb->url.size == new_url_size);
  return AZ_OK;
}

AZ_NODISCARD az_result az_http_request_builder_append_header(
    az_http_request_builder * const p_hrb,
    az_const_span const name,
    az_const_span const value) {
  AZ_CONTRACT_ARG_NOT_NULL(p_hrb);

  if (!az_const_span_is_valid(name) || !az_const_span_is_valid(value) || name.size == 0
      || value.size == 0) {
    return AZ_ERROR_ARG;
  }

  az_pair * const headers
      = get_headers_start(p_hrb->buffer, p_hrb->max_url_size, p_hrb->max_headers);

  if (p_hrb->headers_end >= p_hrb->max_headers || headers == NULL) {
    return AZ_ERROR_BUFFER_OVERFLOW;
  }

  headers[p_hrb->headers_end] = (az_pair){ .key = name, .value = value };
  ++(p_hrb->headers_end);

  return AZ_OK;
}

AZ_NODISCARD az_result az_http_request_builder_mark_retry_headers_start(az_http_request_builder * const p_hrb) {
  p_hrb->retry_headers_start = p_hrb->headers_end;
  return AZ_OK;
}

AZ_NODISCARD az_result az_http_request_builder_remove_retry_headers(az_http_request_builder * const p_hrb) {
  if (p_hrb->headers_end > p_hrb->retry_headers_start) {
    p_hrb->headers_end = p_hrb->retry_headers_start;
  }

  return AZ_OK;
}

az_result az_http_request_builder_get_header(
    az_http_request_builder * const p_hrb,
    uint16_t const index,
    az_pair * const out_result) {
  AZ_CONTRACT_ARG_NOT_NULL(p_hrb);
  AZ_CONTRACT_ARG_NOT_NULL(out_result);

  az_pair * const headers
      = get_headers_start(p_hrb->buffer, p_hrb->max_url_size, p_hrb->max_headers);

  if (index >= p_hrb->headers_end || headers == NULL) {
    return AZ_ERROR_ARG;
  }

  *out_result = headers[index];
  return AZ_OK;
}

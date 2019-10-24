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

AZ_NODISCARD az_result az_http_request_builder_init(
    az_http_request_builder * const p_hrb,
    az_span const buffer,
    az_const_span const method_verb,
    az_const_span const initial_url,
    uint16_t const max_url_size,
    uint16_t const max_headers) {
  AZ_CONTRACT_ARG_NOT_NULL(p_hrb);

  size_t const method_verb_and_url_size = method_verb.size + max_url_size;
  if (!az_span_is_valid(buffer) || !az_const_span_is_valid(method_verb)
      || !az_const_span_is_valid(initial_url) || max_url_size < initial_url.size
      || method_verb_and_url_size < method_verb.size || method_verb_and_url_size < max_url_size
      || az_const_span_is_overlap(az_span_to_const_span(buffer), method_verb)
      || az_const_span_is_overlap(az_span_to_const_span(buffer), initial_url)) {
    return AZ_ERROR_ARG;
  }

  if (buffer.size < max_url_size) {
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

  if (p_hrb->headers_end >= p_hrb->max_headers) {
    return AZ_ERROR_BUFFER_OVERFLOW;
  }

  if (!az_const_span_is_valid(name) || !az_const_span_is_valid(value) || name.size == 0
      || value.size == 0) {
    return AZ_ERROR_ARG;
  }

  for (size_t i = 0; i < name.size; ++i) {
    if (name.begin[i] == '\0') {
      return AZ_ERROR_ARG;
    }
  }

  for (size_t i = 0; i < value.size; ++i) {
    if (value.begin[i] == '\0') {
      return AZ_ERROR_ARG;
    }
  }

  uint8_t * const buffer_end = p_hrb->buffer.begin + p_hrb->buffer.size;
  uint8_t * new_header_start = buffer_end;
  {
    size_t const separator_sizes = sizeof((uint8_t)'\0') + (2 * sizeof((uint8_t)'\0'));

    uint8_t * const headers_start = p_hrb->url.begin + p_hrb->max_url_size;

    size_t const headers_space = buffer_end - headers_start;
    if (headers_start >= buffer_end || headers_space < separator_sizes) {
      return AZ_ERROR_BUFFER_OVERFLOW;
    }

    if (p_hrb->headers_end == 0) {
      new_header_start = headers_start;
    } else {
      uint16_t nheader = 1;
      for (size_t i = 0; i < headers_space - separator_sizes; ++i) {
        if (headers_start[i] == '\0' && headers_start[i + 1] == '\0') {
          assert(i < headers_space);
          assert(i + 1 < headers_space);
          assert(i + 2 < headers_space);

          if (nheader >= p_hrb->headers_end) {
            new_header_start = headers_start + (i + sizeof((uint8_t)'\0')) + 1;
            break;
          }

          ++nheader;
        }
      }
    }

    size_t const available_space = buffer_end - new_header_start;

    size_t const name_and_value_size = name.size + value.size;
    size_t const required_space = name_and_value_size + separator_sizes;

    if (name_and_value_size < name.size || name_and_value_size < value.size
        || required_space < name_and_value_size || required_space > available_space) {
      return AZ_ERROR_BUFFER_OVERFLOW;
    }

    az_const_span const new_header_span = { .begin = new_header_start, .size = required_space };
    if (az_const_span_is_overlap(new_header_span, name)
        || az_const_span_is_overlap(new_header_span, value)) {
      return AZ_ERROR_ARG;
    }
  }

  assert(new_header_start != buffer_end);

  memcpy(new_header_start, name.begin, name.size);
  new_header_start += name.size;

  *new_header_start = '\0';
  ++new_header_start;

  memcpy(new_header_start, value.begin, value.size);
  new_header_start += value.size;

  new_header_start[0] = '\0';
  new_header_start[1] = '\0';

  ++(p_hrb->headers_end);

  return AZ_OK;
}

AZ_NODISCARD az_result az_http_request_builder_mark_retry_headers_start(az_http_request_builder * const p_hrb) {
  p_hrb->retry_headers_start = p_hrb->headers_end;
  return AZ_OK;
}

AZ_NODISCARD az_result az_http_request_builder_remove_retry_headers(az_http_request_builder * const p_hrb) {
  p_hrb->headers_end = p_hrb->retry_headers_start;
  return AZ_OK;
}

az_result az_http_request_builder_get_header(
    az_http_request_builder * const p_hrb,
    uint16_t const index,
    az_pair * const out_result) {
  AZ_CONTRACT_ARG_NOT_NULL(p_hrb);
  AZ_CONTRACT_ARG_NOT_NULL(out_result);

  if (index >= p_hrb->headers_end) {
    return AZ_ERROR_ARG;
  }

  uint8_t * const buffer_end = p_hrb->buffer.begin + p_hrb->buffer.size;
  uint8_t * const headers_start = p_hrb->url.begin + p_hrb->max_url_size;

  size_t const headers_space = buffer_end - headers_start;

  if (headers_start >= buffer_end || headers_space < (2 * sizeof((uint8_t)'\0'))) {
    return AZ_ERROR_ARG;
  }

  uint16_t nheader = 0;
  uint8_t * last_header_start = headers_start;
  uint8_t * last_header_name_end = last_header_start;
  for (size_t i = 0; i < headers_space - (2 * sizeof((uint8_t)'\0')); ++i) {
    if (headers_start[i] == '\0')
      if (headers_start[i + 1] != '\0') {
        last_header_name_end = headers_start + i;
      } else {
        if (nheader == index) {
          *out_result = (az_pair){
            .key = { .begin = last_header_start, .size = last_header_name_end - last_header_start },
            .value
            = { .begin = last_header_name_end + sizeof((uint8_t)'\0'),
                .size = (headers_start + i) - (last_header_name_end + sizeof((uint8_t)'\0')) }
          };
          return AZ_OK;
        }

        ++nheader;
        last_header_start = headers_start + (i + sizeof((uint8_t)'\0')) + 1;
        last_header_name_end = last_header_start;
      }
  }

  return AZ_ERROR_ARG;
}

// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#ifndef _az_HTTP_H
#define _az_HTTP_H

#include <az_result.h>
#include <az_span.h>
#include <az_span_reader.h>

#include <stdint.h>

#include <_az_cfg_prefix.h>

typedef struct {
  uint16_t max_retry;
  uint16_t delay_in_ms;
} az_http_policy_retry_options;

typedef struct {
  az_span buffer;
  az_span method_verb;
  az_span url_builder;
  int16_t max_headers;
  int32_t retry_headers_start;
  int32_t headers_end;
  az_span body;
  int32_t query_start;
} az_http_request_builder;

extern az_span const AZ_HTTP_METHOD_VERB_GET;
extern az_span const AZ_HTTP_METHOD_VERB_HEAD;
extern az_span const AZ_HTTP_METHOD_VERB_POST;
extern az_span const AZ_HTTP_METHOD_VERB_PUT;
extern az_span const AZ_HTTP_METHOD_VERB_DELETE;
extern az_span const AZ_HTTP_METHOD_VERB_TRACE;
extern az_span const AZ_HTTP_METHOD_VERB_OPTIONS;
extern az_span const AZ_HTTP_METHOD_VERB_CONNECT;
extern az_span const AZ_HTTP_METHOD_VERB_PATCH;

/**
 * @brief Format buffer as a http request containing URL and header spans.
 *
 * @param p_hrb HTTP request builder to initialize.
 * @param buffer Buffer to store URL and header spans in.
 * @param max_url_size Maximum URL length (see @ref az_http_request_builder_set_query_parameter).
 * @param method_verb HTTP verb: `"GET"`, `"POST"`, etc.
 * @param initial_url URL.
 *
 * @return
 *   - *`AZ_OK`* success.
 *   - *`AZ_ERROR_BUFFER_OVERFLOW`* `buffer` does not have enough space to fit the `max_url_size`.
 *   - *`AZ_ERROR_ARG`*
 *     - `p_hrb` is _NULL_.
 *     - `buffer`, `method_verb`, or `initial_url` are invalid spans (see @ref az_span_is_valid).
 *     - `max_url_size` is less than `initial_url.size`.
 */
AZ_NODISCARD az_result az_http_request_builder_init(
    az_http_request_builder * p_hrb,
    az_span buffer,
    int32_t max_url_size,
    az_span method_verb,
    az_span initial_url,
    az_span body);

/**
 * @brief Set query parameter.
 *
 * @param p_hrb HTTP request builder that holds the URL to set the query parameter to.
 * @param name URL parameter name.
 * @param value URL parameter value.
 *
 * @return
 *   - *`AZ_OK`* success.
 *   - *`AZ_ERROR_BUFFER_OVERFLOW`* the `URL` would grow past the `max_url_size`, should the
 * parameter gets set.
 *   - *`AZ_ERROR_ARG`*
 *     - `p_hrb` is _NULL_.
 *     - `name` or `value` are invalid spans (see @ref az_span_is_valid).
 *     - `name` or `value` are empty.
 *     - `name`'s or `value`'s buffer overlap resulting `url`'s buffer.
 */
AZ_NODISCARD az_result az_http_request_builder_set_query_parameter(
    az_http_request_builder * p_hrb,
    az_span name,
    az_span value);

/**
 * @brief Add a new HTTP header for the request.
 *
 * @param p_hrb HTTP request builder that holds the URL to set the query parameter to.
 * @param key Header name (e.g. `"Content-Type"`).
 * @param value Header value (e.g. `"application/x-www-form-urlencoded"`).
 *
 * @return
 *   - *`AZ_OK`* success.
 *   - *`AZ_ERROR_BUFFER_OVERFLOW`* there isn't enough space in the `p_hrb->buffer` to add a header.
 *   - *`AZ_ERROR_ARG`*
 *     - `p_hrb` is _NULL_.
 *     - `key` or `value` are invalid spans (see @ref az_span_is_valid).
 *     - `key` or `value` are empty.
 *     - `name`'s or `value`'s buffer overlap resulting `url`'s buffer.
 */
AZ_NODISCARD az_result
az_http_request_builder_append_header(az_http_request_builder * p_hrb, az_span key, az_span value);

/**
 * @brief Adds path to url request.
 * For instance, if url in request is `http://example.net?qp=1` and this function is called with
 * path equals to `test`, then request url will be updated to `http://example.net/test?qp=1`.
 *
 *
 * @param p_hrb http request builder reference
 * @param path span to a path to be appended into url
 * @return AZ_NODISCARD az_http_request_builder_append_path
 */
AZ_NODISCARD az_result
az_http_request_builder_append_path(az_http_request_builder * p_hrb, az_span path);

/**
 * @brief Mark that the HTTP headers that are gong to be added via
 * `az_http_request_builder_append_header` are going to be considered as retry headers.
 *
 * @param p_hrb HTTP request builder.
 *
 * @return
 *   - *`AZ_OK`* success.
 *   - *`AZ_ERROR_ARG`* `p_hrb` is _NULL_.
 */
AZ_NODISCARD az_result
az_http_request_builder_mark_retry_headers_start(az_http_request_builder * p_hrb);

/**
 * @brief Drop all the HTTP headers that were marked as retry headers. No-op if none were marked.
 *
 * @param p_hrb HTTP request builder.
 *
 * @return
 *   - *`AZ_OK`* success.
 *   - *`AZ_ERROR_ARG`* `p_hrb` is _NULL_.
 */
AZ_NODISCARD az_result
az_http_request_builder_remove_retry_headers(az_http_request_builder * p_hrb);

/**
 * @brief Get the HTTP header by index.
 *
 * @param p_hrb HTTP request builder.
 * @param index Index of the HTTP header to get from the builder.
 * @param out_result Pointer to write the result to.
 *
 * @return
 *   - *`AZ_OK`* success.
 *   - *`AZ_ERROR_ARG`*
 *     - `p_hrb` or `out_result` are _NULL_.
 *     - `index` is out of range.
 */
AZ_NODISCARD az_result az_http_request_builder_get_header(
    az_http_request_builder * p_hrb,
    uint16_t index,
    az_pair * out_result);

/**
 * @brief utility function for checking if there is at least one header in the request
 *
 */
AZ_NODISCARD AZ_INLINE bool az_http_request_builder_has_headers(az_http_request_builder * p_hrb) {
  return p_hrb->headers_end > 0;
}

typedef enum {
  AZ_HTTP_RESPONSE_KIND_NONE = 0,
  AZ_HTTP_RESPONSE_KIND_STATUS_LINE = 1,
  AZ_HTTP_RESPONSE_KIND_HEADER = 2,
  AZ_HTTP_RESPONSE_KIND_BODY = 3,
} az_http_response_kind;

typedef struct {
  struct {
    az_span http_response;

    az_span reader; //  index into http_response   //az_span_reader reader;
    az_http_response_kind kind;
  } _internal;
} az_http_response;

///////////////////////////////////////////////

typedef enum {
  AZ_HTTP_STATUS_CODE_OK = 200,
  AZ_HTTP_STATUS_CODE_NOT_FOUND = 404,
} az_http_status_code;

/**
 * An HTTP response status line
 *
 * See https://tools.ietf.org/html/rfc7230#section-3.1.2
 */
typedef struct {
  uint8_t major_version;
  uint8_t minor_version;
  az_http_status_code status_code;
  az_span reason_phrase;
} az_http_response_status_line;

AZ_NODISCARD AZ_INLINE az_result
az_http_response_init(az_http_response * self, az_span http_response) {
  *self = (az_http_response){ ._internal = { .http_response = http_response,
                                             .reader = az_span_null(),
                                             .kind = AZ_HTTP_RESPONSE_KIND_NONE } };
  return AZ_OK;
}

/**
 * @brief Set the az_http_response_parser to index zero inside az_http_response and tries to get
 * status line from it.
 *
 * @param response az_http_response with an http response
 * @param out inline code from http response
 * @return AZ_OK when inline code is parsed and returned. AZ_ERROR if http response was not parsed
 */
AZ_NODISCARD az_result
az_http_response_get_status_line(az_http_response * response, az_http_response_status_line * out);

/**
 * @brief parse a header based on the last http response parsed.
 * If called right after parsin status line, this function will try to get the first header from
 * http response.
 * If called right after parsing a header, this function will try to get
 * another header from http response or will return AZ_ERROR_ITEM_NOT_FOUND if there are no more
 * headers.
 * If called after parsing http body or before parsing status line, this function will return
 * AZ_ERROR_INVALID_STATE
 *
 * @param self an HTTP response
 * @param out an az_pair containing a header when az_result is AZ_OK
 * @return AZ_OK if a header was parsed. See above for returned Errors.
 */
AZ_NODISCARD az_result az_http_response_get_next_header(az_http_response * self, az_pair * out);

/**
 * @brief parses http response body and make out_body point to it.
 * This function can be called directly and status line and headers are parsed and ignored first
 *
 * @param self an http response
 * @param out_body out parameter to point to http response body
 * @return AZ_NODISCARD az_http_response_get_body
 */
AZ_NODISCARD az_result az_http_response_get_body(az_http_response * self, az_span * out_body);

AZ_NODISCARD AZ_INLINE az_result az_http_response_reset(az_http_response * const self) {
  self->_internal.http_response = az_span_init(
      az_span_ptr(self->_internal.http_response),
      0,
      az_span_capacity(self->_internal.http_response));
  return AZ_OK;
}

#include <_az_cfg_suffix.h>

#endif

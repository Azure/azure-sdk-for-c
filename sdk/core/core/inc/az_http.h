// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#ifndef _az_HTTP_H
#define _az_HTTP_H

#include <az_result.h>
#include <az_span.h>
#include <az_config.h>

#include <stdint.h>

#include <_az_cfg_prefix.h>

typedef enum {
  az_http_policy_apiversion_option_location_header,
  az_http_policy_apiversion_option_location_queryparameter
} az_http_policy_apiversion_option_location;

typedef struct {
  // Services pass API versions in the header or in query parameters
  az_http_policy_apiversion_option_location option_location;
  az_span name;
  az_span version;
} _az_http_policy_apiversion_options;

AZ_NODISCARD AZ_INLINE _az_http_policy_apiversion_options
az_http_policy_apiversion_options_default() {
  return (_az_http_policy_apiversion_options){ 0 };
}

/**
 * @brief options for the telemetry policy
 * os = string representation of currently executing Operating System
 *
 */
typedef struct {
  az_span os;
} _az_http_policy_telemetry_options;

AZ_NODISCARD AZ_INLINE _az_http_policy_telemetry_options
_az_http_policy_apiversion_options_default() {
  return (_az_http_policy_telemetry_options){ .os = AZ_SPAN_FROM_STR("Unknown OS") };
}

typedef struct {
  uint16_t max_retries;
  uint16_t delay_in_ms;
  uint16_t max_delay_in_ms; // TODO: review naming for this
  // TODO: List of HTTP status code to be added
} az_http_policy_retry_options;

AZ_NODISCARD AZ_INLINE az_http_policy_retry_options az_http_policy_retry_options_default() {
  return (az_http_policy_retry_options){
    .max_retries = 3,
    .delay_in_ms = 10,
    .max_delay_in_ms = 30, // TODO: adjust this numbers
  };
}

typedef az_span az_http_method;

typedef struct {
  struct {
    az_http_method method;
    az_span url;
    int32_t query_start;
    az_span headers;
    int32_t max_headers;
    int32_t retry_headers_start_byte_offset;
    // int32_t headers_end;
    az_span body;
  } _internal;
} az_http_request;

AZ_INLINE az_http_method az_http_method_get() { return AZ_SPAN_FROM_STR("GET"); }
AZ_INLINE az_http_method az_http_method_head() { return AZ_SPAN_FROM_STR("HEAD"); }
AZ_INLINE az_http_method az_http_method_post() { return AZ_SPAN_FROM_STR("POST"); }
AZ_INLINE az_http_method az_http_method_put() { return AZ_SPAN_FROM_STR("PUT"); }
AZ_INLINE az_http_method az_http_method_delete() { return AZ_SPAN_FROM_STR("DELETE"); }
AZ_INLINE az_http_method az_http_method_patch() { return AZ_SPAN_FROM_STR("PATCH"); }

/**
 * @brief Format buffer as a http request containing URL and header spans.
 *
 * @param p_request HTTP request builder to initialize.
 * @param method HTTP verb: `"GET"`, `"POST"`, etc.
 * @param url Maximum URL length (see @ref az_http_request_builder_set_query_parameter).
 * @param headers_buffer HTTP verb: `"GET"`, `"POST"`, etc.
 * @param body URL.
 *
 * @return
 *   - *`AZ_OK`* success.
 *   - *`AZ_ERROR_BUFFER_OVERFLOW`* `buffer` does not have enough space to fit the `max_url_size`.
 *   - *`AZ_ERROR_ARG`*
 *     - `p_request` is _NULL_.
 *     - `buffer`, `method_verb`, or `initial_url` are invalid spans (see @ref az_span_is_valid).
 *     - `max_url_size` is less than `initial_url.size`.
 */
AZ_NODISCARD az_result az_http_request_init(
    az_http_request * p_request,
    az_http_method method,
    az_span url,
    az_span headers_buffer,
    az_span body);

/**
 * @brief Adds path to url request.
 * For instance, if url in request is `http://example.net?qp=1` and this function is called with
 * path equals to `test`, then request url will be updated to `http://example.net/test?qp=1`.
 *
 *
 * @param p_request http request builder reference
 * @param path span to a path to be appended into url
 * @return AZ_NODISCARD az_http_request_builder_append_path
 */
AZ_NODISCARD az_result az_http_request_append_path(az_http_request * p_request, az_span path);

/**
 * @brief Set query parameter.
 *
 * @param p_request HTTP request builder that holds the URL to set the query parameter to.
 * @param name URL parameter name.
 * @param value URL parameter value.
 *
 * @return
 *   - *`AZ_OK`* success.
 *   - *`AZ_ERROR_BUFFER_OVERFLOW`* the `URL` would grow past the `max_url_size`, should the
 * parameter gets set.
 *   - *`AZ_ERROR_ARG`*
 *     - `p_request` is _NULL_.
 *     - `name` or `value` are invalid spans (see @ref az_span_is_valid).
 *     - `name` or `value` are empty.
 *     - `name`'s or `value`'s buffer overlap resulting `url`'s buffer.
 */
AZ_NODISCARD az_result
az_http_request_set_query_parameter(az_http_request * p_request, az_span name, az_span value);

/**
 * @brief Add a new HTTP header for the request.
 *
 * @param p_request HTTP request builder that holds the URL to set the query parameter to.
 * @param key Header name (e.g. `"Content-Type"`).
 * @param value Header value (e.g. `"application/x-www-form-urlencoded"`).
 *
 * @return
 *   - *`AZ_OK`* success.
 *   - *`AZ_ERROR_BUFFER_OVERFLOW`* there isn't enough space in the `p_request->buffer` to add a
 * header.
 *   - *`AZ_ERROR_ARG`*
 *     - `p_request` is _NULL_.
 *     - `key` or `value` are invalid spans (see @ref az_span_is_valid).
 *     - `key` or `value` are empty.
 *     - `name`'s or `value`'s buffer overlap resulting `url`'s buffer.
 */
AZ_NODISCARD az_result
az_http_request_append_header(az_http_request * p_request, az_span key, az_span value);

/**
 * @brief Get the HTTP header by index.
 *
 * @param p_request HTTP request builder.
 * @param index Index of the HTTP header to get from the builder.
 * @param out_result Pointer to write the result to.
 *
 * @return
 *   - *`AZ_OK`* success.
 *   - *`AZ_ERROR_ARG`*
 *     - `p_request` or `out_result` are _NULL_.
 *     - `index` is out of range.
 */
AZ_NODISCARD az_result
az_http_request_get_header(az_http_request * p_request, int32_t index, az_pair * out_result);

typedef enum {
  AZ_HTTP_RESPONSE_KIND_STATUS_LINE = 0,
  AZ_HTTP_RESPONSE_KIND_HEADER = 1,
  AZ_HTTP_RESPONSE_KIND_BODY = 2,
  AZ_HTTP_RESPONSE_KIND_EOF = 3,
} az_http_response_kind;

typedef struct {
  struct {
    az_span http_response;
    struct {
      az_span remaining; // the remaining un-parsed portion of the original http_response.
      az_http_response_kind next_kind; // after parsing an element, this is set to the next kind of
                                       // thing we will be parsing.
    } parser;
  } _internal;
} az_http_response;
///////////////////////////////////////////////

typedef enum {
  // 1xx (information) Status Codes:
  AZ_HTTP_STATUS_CODE_CONTINUE = 100,
  AZ_HTTP_STATUS_CODE_SWITCHING_PROTOCOLS = 101,
  AZ_HTTP_STATUS_CODE_PROCESSING = 102,
  AZ_HTTP_STATUS_CODE_EARLYPROCESSING = 103,

  // 2xx (successful) Status Codes:
  AZ_HTTP_STATUS_CODE_OK = 200,
  AZ_HTTP_STATUS_CODE_CREATED = 201,
  AZ_HTTP_STATUS_CODE_ACCEPTED = 202,
  AZ_HTTP_STATUS_CODE_NON_AUTHORITATIVE_INFORMATION = 203,
  AZ_HTTP_STATUS_CODE_NO_CONTENT = 204,
  AZ_HTTP_STATUS_CODE_RESET_CONTENT = 205,
  AZ_HTTP_STATUS_CODE_PARTIAL_CONTENT = 206,
  AZ_HTTP_STATUS_CODE_MULTI_STATUS = 207,
  AZ_HTTP_STATUS_CODE_ALREADY_REPORTED = 208,
  AZ_HTTP_STATUS_CODE_IM_USED = 226,

  // 3xx (redirection) Status Codes:
  AZ_HTTP_STATUS_CODE_MULTIPLE_CHOICES = 300,
  AZ_HTTP_STATUS_CODE_MOVED_PERMANENTLY = 301,
  AZ_HTTP_STATUS_CODE_FOUND = 302,
  AZ_HTTP_STATUS_CODE_SEE_OTHER = 303,
  AZ_HTTP_STATUS_CODE_NOT_MODIFIED = 304,
  AZ_HTTP_STATUS_CODE_USE_PROXY = 305,
  AZ_HTTP_STATUS_CODE_TEMPORARY_REDIRECT = 307,
  AZ_HTTP_STATUS_CODE_PERMANENT_REDIRECT = 308,

  // 4xx (client error) Status Codes:
  AZ_HTTP_STATUS_CODE_BAD_REQUEST = 400,
  AZ_HTTP_STATUS_CODE_UNAUTHORIZED = 401,
  AZ_HTTP_STATUS_CODE_PAYMENT_REQUIRED = 402,
  AZ_HTTP_STATUS_CODE_FORBIDDEN = 403,
  AZ_HTTP_STATUS_CODE_NOT_FOUND = 404,
  AZ_HTTP_STATUS_CODE_METHOD_NOT_ALLOWED = 405,
  AZ_HTTP_STATUS_CODE_NOT_ACCEPTABLE = 406,
  AZ_HTTP_STATUS_CODE_PROXY_AUTHENTICATION_REQUIRED = 407,
  AZ_HTTP_STATUS_CODE_REQUEST_TIMEOUT = 408,
  AZ_HTTP_STATUS_CODE_CONFLICT = 409,
  AZ_HTTP_STATUS_CODE_GONE = 410,
  AZ_HTTP_STATUS_CODE_LENGTH_REQUIRED = 411,
  AZ_HTTP_STATUS_CODE_PRECONDITION_FAILED = 412,
  AZ_HTTP_STATUS_CODE_REQUEST_ENTITY_TOO_LARGE = 413,
  AZ_HTTP_STATUS_CODE_REQUEST_URI_TOO_LONG = 414,
  AZ_HTTP_STATUS_CODE_UNSUPPORTED_MEDIA_TYPE = 415,
  AZ_HTTP_STATUS_CODE_REQUESTED_RANGE_NOT_SATISFIABLE = 416,
  AZ_HTTP_STATUS_CODE_EXPECTATION_FAILED = 417,
  AZ_HTTP_STATUS_CODE_IM_A_TEAPOT = 418,
  AZ_HTTP_STATUS_CODE_ENHANCE_YOUR_CALM = 420,
  AZ_HTTP_STATUS_CODE_UNPROCESSABLE_ENTITY = 422,
  AZ_HTTP_STATUS_CODE_LOCKED = 423,
  AZ_HTTP_STATUS_CODE_FAILED_DEPENDENCY = 424,
  AZ_HTTP_STATUS_CODE_UPGRADE_REQUIRED = 426,
  AZ_HTTP_STATUS_CODEPRECONDITION_REQUIRED_ = 428,
  AZ_HTTP_STATUS_CODE_TOO_MANY_REQUESTS = 429,
  AZ_HTTP_STATUS_CODE_REQUEST_HEADER_FIELDS_TOO_LARGE = 431,
  AZ_HTTP_STATUS_CODE_NO_RESPONSE = 444,
  AZ_HTTP_STATUS_CODE_RETRY_WITH = 449,
  AZ_HTTP_STATUS_CODE_BLOCKED_BY_wINDOWS_PARENTAL_cONTROLS = 450,
  AZ_HTTP_STATUS_CODE_UNAVAILABLE_FOR_LEGAL_REASONS = 451,
  AZ_HTTP_STATUS_CODE_CLIENT_CLOSED_REQUEST = 499,

  // 5xx (SERVER error) Status Codes:
  AZ_HTTP_STATUS_CODE_INTERLAN_SERVER_ERROR = 500,
  AZ_HTTP_STATUS_CODE_NOT_IMPLEMENTED = 501,
  AZ_HTTP_STATUS_CODE_BAD_GATEWAY = 502,
  AZ_HTTP_STATUS_CODE_SERVICE_UNAVAILABLE = 503,
  AZ_HTTP_STATUS_CODE_GATEWAY_TIMEOUT = 504,
  AZ_HTTP_STATUS_CODE_HTTP_VERSION_NOT_SUPPORTED = 505,
  AZ_HTTP_STATUS_CODE_VARIANT_ALSO_NEGOTIATES = 506,
  AZ_HTTP_STATUS_CODE_INSUFFICIENT_STORAGE = 507,
  AZ_HTTP_STATUS_CODE_LOOP_DETECTED = 508,
  AZ_HTTP_STATUS_CODE_BANDWIDTH_LIMIT_EXCEEDED = 509,
  AZ_HTTP_STATUS_CODE_NOT_EXTENDED = 510,
  AZ_HTTP_STATUS_CODE_NETWORK_AUTHENTICATION_REQUIRED = 511,
  AZ_HTTP_STATUS_CODE_NETWORK_READ_TIMEOUT_ERROR = 598,
  AZ_HTTP_STATUS_CODE_NETWORK_CONNECT_TIMEOUT_ERROR = 599,
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
  *self = (az_http_response){ ._internal
                              = { .http_response = http_response,
                                  .parser = { .remaining = az_span_null(),
                                              .next_kind = AZ_HTTP_RESPONSE_KIND_STATUS_LINE } } };
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

AZ_NODISCARD AZ_INLINE az_result az_http_response_reset(az_http_response * self) {
  self->_internal.http_response = az_span_init(
      az_span_ptr(self->_internal.http_response),
      0,
      az_span_capacity(self->_internal.http_response));
  return AZ_OK;
}

typedef AZ_NODISCARD az_result (
    *az_http_client_fn)(az_http_request * p_request, az_http_response * p_response);

#include <_az_cfg_suffix.h>

#endif

// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#ifndef _az_HTTP_H
#define _az_HTTP_H

#include <az_result.h>
#include <az_span.h>
#include <az_config.h>

#include <stdbool.h>
#include <stdint.h>

#include <_az_cfg_prefix.h>

typedef enum {
  _az_http_policy_apiversion_option_location_header,
  _az_http_policy_apiversion_option_location_queryparameter
} _az_http_policy_apiversion_option_location;

typedef az_span az_http_method;

typedef struct {
  struct {
    az_http_method method;
    az_span url;
    int32_t query_start;
    az_span headers;
    int32_t max_headers;
    int32_t retry_headers_start_byte_offset;
    az_span body;
  } _internal;
} _az_http_request;

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

// Required to define az_http_policy for using it to create policy process definition
typedef struct _az_http_policy _az_http_policy;

typedef AZ_NODISCARD az_result (*_az_http_policy_process_fn)(
    _az_http_policy * p_policies,
    void * p_options,
    _az_http_request * p_request,
    az_http_response * p_response);

struct _az_http_policy {
  struct {
    _az_http_policy_process_fn process;
    void * p_options;
  } _internal;
};

typedef struct {
  struct {
    _az_http_policy p_policies[10];
  } _internal;
} _az_http_pipeline;

typedef struct {
  // Services pass API versions in the header or in query parameters
  struct {
    _az_http_policy_apiversion_option_location option_location;
    az_span name;
    az_span version;
  } _internal;
} _az_http_policy_apiversion_options;

/**
 * @brief options for the telemetry policy
 * os = string representation of currently executing Operating System
 *
 */
typedef struct {
  az_span os;
} _az_http_policy_telemetry_options;

AZ_NODISCARD AZ_INLINE _az_http_policy_telemetry_options
_az_http_policy_telemetry_options_default() {
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

typedef enum {
  // 1xx (information) Status Codes:
  _AZ_HTTP_STATUS_CODE_CONTINUE = 100,
  _AZ_HTTP_STATUS_CODE_SWITCHING_PROTOCOLS = 101,
  _AZ_HTTP_STATUS_CODE_PROCESSING = 102,
  _AZ_HTTP_STATUS_CODE_EARLY_HINTS = 103,

  // 2xx (successful) Status Codes:
  _AZ_HTTP_STATUS_CODE_OK = 200,
  _AZ_HTTP_STATUS_CODE_CREATED = 201,
  _AZ_HTTP_STATUS_CODE_ACCEPTED = 202,
  _AZ_HTTP_STATUS_CODE_NON_AUTHORITATIVE_INFORMATION = 203,
  _AZ_HTTP_STATUS_CODE_NO_CONTENT = 204,
  _AZ_HTTP_STATUS_CODE_RESET_CONTENT = 205,
  _AZ_HTTP_STATUS_CODE_PARTIAL_CONTENT = 206,
  _AZ_HTTP_STATUS_CODE_MULTI_STATUS = 207,
  _AZ_HTTP_STATUS_CODE_ALREADY_REPORTED = 208,
  _AZ_HTTP_STATUS_CODE_IM_USED = 226,

  // 3xx (redirection) Status Codes:
  _AZ_HTTP_STATUS_CODE_MULTIPLE_CHOICES = 300,
  _AZ_HTTP_STATUS_CODE_MOVED_PERMANENTLY = 301,
  _AZ_HTTP_STATUS_CODE_FOUND = 302,
  _AZ_HTTP_STATUS_CODE_SEE_OTHER = 303,
  _AZ_HTTP_STATUS_CODE_NOT_MODIFIED = 304,
  _AZ_HTTP_STATUS_CODE_USE_PROXY = 305,
  _AZ_HTTP_STATUS_CODE_TEMPORARY_REDIRECT = 307,
  _AZ_HTTP_STATUS_CODE_PERMANENT_REDIRECT = 308,

  // 4xx (client error) Status Codes:
  _AZ_HTTP_STATUS_CODE_BAD_REQUEST = 400,
  _AZ_HTTP_STATUS_CODE_UNAUTHORIZED = 401,
  _AZ_HTTP_STATUS_CODE_PAYMENT_REQUIRED = 402,
  _AZ_HTTP_STATUS_CODE_FORBIDDEN = 403,
  _AZ_HTTP_STATUS_CODE_NOT_FOUND = 404,
  _AZ_HTTP_STATUS_CODE_METHOD_NOT_ALLOWED = 405,
  _AZ_HTTP_STATUS_CODE_NOT_ACCEPTABLE = 406,
  _AZ_HTTP_STATUS_CODE_PROXY_AUTHENTICATION_REQUIRED = 407,
  _AZ_HTTP_STATUS_CODE_REQUEST_TIMEOUT = 408,
  _AZ_HTTP_STATUS_CODE_CONFLICT = 409,
  _AZ_HTTP_STATUS_CODE_GONE = 410,
  _AZ_HTTP_STATUS_CODE_LENGTH_REQUIRED = 411,
  _AZ_HTTP_STATUS_CODE_PRECONDITION_FAILED = 412,
  _AZ_HTTP_STATUS_CODE_PAYLOAD_TOO_LARGE = 413,
  _AZ_HTTP_STATUS_CODE_URI_TOO_LONG = 414,
  _AZ_HTTP_STATUS_CODE_UNSUPPORTED_MEDIA_TYPE = 415,
  _AZ_HTTP_STATUS_CODE_RANGE_NOT_SATISFIABLE = 416,
  _AZ_HTTP_STATUS_CODE_EXPECTATION_FAILED = 417,
  _AZ_HTTP_STATUS_CODE_MISDIRECTED_REQUEST = 421,
  _AZ_HTTP_STATUS_CODE_UNPROCESSABLE_ENTITY = 422,
  _AZ_HTTP_STATUS_CODE_LOCKED = 423,
  _AZ_HTTP_STATUS_CODE_FAILED_DEPENDENCY = 424,
  _AZ_HTTP_STATUS_CODE_TOO_EARLY = 425,
  _AZ_HTTP_STATUS_CODE_UPGRADE_REQUIRED = 426,
  _AZ_HTTP_STATUS_CODE_PRECONDITION_REQUIRED = 428,
  _AZ_HTTP_STATUS_CODE_TOO_MANY_REQUESTS = 429,
  _AZ_HTTP_STATUS_CODE_REQUEST_HEADER_FIELDS_TOO_LARGE = 431,
  _AZ_HTTP_STATUS_CODE_UNAVAILABLE_FOR_LEGAL_REASONS = 451,

  // 5xx (server error) Status Codes:
  _AZ_HTTP_STATUS_CODE_INTERNAL_SERVER_ERROR = 500,
  _AZ_HTTP_STATUS_CODE_NOT_IMPLEMENTED = 501,
  _AZ_HTTP_STATUS_CODE_BAD_GATEWAY = 502,
  _AZ_HTTP_STATUS_CODE_SERVICE_UNAVAILABLE = 503,
  _AZ_HTTP_STATUS_CODE_GATEWAY_TIMEOUT = 504,
  _AZ_HTTP_STATUS_CODE_HTTP_VERSION_NOT_SUPPORTED = 505,
  _AZ_HTTP_STATUS_CODE_VARIANT_ALSO_NEGOTIATES = 506,
  _AZ_HTTP_STATUS_CODE_INSUFFICIENT_STORAGE = 507,
  _AZ_HTTP_STATUS_CODE_LOOP_DETECTED = 508,
  _AZ_HTTP_STATUS_CODE_NOT_EXTENDED = 510,
  _AZ_HTTP_STATUS_CODE_NETWORK_AUTHENTICATION_REQUIRED = 511,
} _az_http_status_code;

/**
 * An HTTP response status line
 *
 * See https://tools.ietf.org/html/rfc7230#section-3.1.2
 */
typedef struct {
  uint8_t major_version;
  uint8_t minor_version;
  _az_http_status_code status_code;
  az_span reason_phrase;
} az_http_response_status_line;

AZ_NODISCARD AZ_INLINE az_result
az_http_response_init(az_http_response * self, az_span http_response) {
  *self = (az_http_response){
    ._internal = {
      .http_response = http_response,
      .parser = {
        .remaining = az_span_null(),
        .next_kind = AZ_HTTP_RESPONSE_KIND_STATUS_LINE,
      },
    },
  };

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
    *az_http_client_send_request_fn)(_az_http_request * p_request, az_http_response * p_response);

typedef struct {
  struct {
    az_http_client_send_request_fn send_request;
  } _internal;
} az_http_transport_options;

#include <_az_cfg_suffix.h>

#endif

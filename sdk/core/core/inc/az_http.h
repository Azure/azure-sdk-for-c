// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

/**
 * @file az_http.h
 *
 * @brief Definition and functionality for az_http_response and related http utilities that allows
 * customer to create required component for sdk clients
 */

#ifndef _az_HTTP_H
#define _az_HTTP_H

#include <az_config.h>
#include <az_result.h>
#include <az_span.h>

#include <stdbool.h>
#include <stdint.h>

#include <_az_cfg_prefix.h>

typedef enum
{
  _az_http_policy_apiversion_option_location_header,
  _az_http_policy_apiversion_option_location_queryparameter
} _az_http_policy_apiversion_option_location;

/**
 * @brief Define az_http_method as an az_span so it is limited to that type and not to any az_span
 *
 */
typedef az_span az_http_method;

/**
 * @brief Defines an az_http_request. This is an internal structure that is used to perform an http
 * request to Azure. It contains an HTTP method, url, headers and body. It also contains another
 * utility variables. User should never access field _internal directly
 *
 */
typedef struct
{
  struct
  {
    az_http_method method;
    az_span url;
    int32_t query_start;
    az_span headers;
    int32_t max_headers;
    int32_t retry_headers_start_byte_offset;
    az_span body;
  } _internal;
} _az_http_request;

typedef enum
{
  AZ_HTTP_RESPONSE_KIND_STATUS_LINE = 0,
  AZ_HTTP_RESPONSE_KIND_HEADER = 1,
  AZ_HTTP_RESPONSE_KIND_BODY = 2,
  AZ_HTTP_RESPONSE_KIND_EOF = 3,
} az_http_response_kind;

/**
 * @brief An HTTP response where SDK client will write Azure services response.
 *
 * Users can access http_response as an az_span to get the http raw response from Netwok. It also
 * contains utilities variables inside _internal to parse response and allow users to get specific
 * response's seccions like status line, headers or body.
 *
 * User @b should @b not access _internal field directly.
 *
 * Use functions az_http_response_get_status_line(), az_http_response_get_next_header() or
 * az_http_response_get_body() as utilities for parsing http response.
 *
 */
typedef struct
{
  struct
  {
    az_span http_response;
    struct
    {
      az_span remaining; // the remaining un-parsed portion of the original http_response.
      az_http_response_kind next_kind; // after parsing an element, this is set to the next kind of
                                       // thing we will be parsing.
    } parser;
  } _internal;
} az_http_response;

/**
 * @brief Declaring az_http_policy for using it to create policy process callback
 * _az_http_policy_process_fn definition. Definition is added below after it.
 *
 */
typedef struct _az_http_policy _az_http_policy;

/**
 * @brief Defines the callback signature of a policy process witch should receive an
 * _az_http_policy, options reference (as void *), an _az_http_request and az_http_response.
 *
 * void * is used as polymorphic solution for any policy. Each policy implementation would know the
 * specif pointer type to cast options to.
 *
 */
typedef AZ_NODISCARD az_result (*_az_http_policy_process_fn)(
    _az_http_policy* p_policies,
    void* p_options,
    _az_http_request* p_request,
    az_http_response* p_response);

/**
 * @brief Definition for an HTTP policy.
 *
 * An HTTP pipeline inside SDK clients is made of an array of this http policies.
 *
 * Users @b should @b not access _internal field where process callback and options are defined.
 *
 */
struct _az_http_policy
{
  struct
  {
    _az_http_policy_process_fn process;
    void* p_options;
  } _internal;
};

/**
 * @brief Internal definition of an HTTP pipeline.
 *
 * Defines the number of policies inside a pipeline.
 *
 * Users @b should @b not access _internal field.
 *
 */
typedef struct
{
  struct
  {
    _az_http_policy p_policies[10];
  } _internal;
} _az_http_pipeline;

/**
 * @brief Defines the options structure used by the api version policy
 *
 * Users @b should @b not access _internal field.
 *
 */
typedef struct
{
  // Services pass API versions in the header or in query parameters
  struct
  {
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
typedef struct
{
  az_span os;
} _az_http_policy_telemetry_options;

/**
 * @brief Initialize _az_http_policy_telemetry_options with default values
 *
 */
AZ_NODISCARD AZ_INLINE _az_http_policy_telemetry_options _az_http_policy_telemetry_options_default()
{
  return (_az_http_policy_telemetry_options){ .os = AZ_SPAN_FROM_STR("Unknown OS") };
}

/**
 * @brief Retry configuration for an HTTP pipeline
 *
 */
typedef struct
{
  uint16_t max_retries;
  uint16_t delay_in_ms;
  uint16_t max_delay_in_ms;
} az_http_policy_retry_options;

/**
 * @brief Initialize az_http_policy_retry_options with default values
 *
 */
AZ_NODISCARD AZ_INLINE az_http_policy_retry_options az_http_policy_retry_options_default()
{
  return (az_http_policy_retry_options){
    .max_retries = 3,
    .delay_in_ms = 10,
    .max_delay_in_ms = 30, // TODO: adjust this numbers
  };
}

typedef enum
{
  // 1xx (information) Status Codes:
  AZ_HTTP_STATUS_CODE_CONTINUE = 100,
  AZ_HTTP_STATUS_CODE_SWITCHING_PROTOCOLS = 101,
  AZ_HTTP_STATUS_CODE_PROCESSING = 102,
  AZ_HTTP_STATUS_CODE_EARLY_HINTS = 103,

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
  AZ_HTTP_STATUS_CODE_PAYLOAD_TOO_LARGE = 413,
  AZ_HTTP_STATUS_CODE_URI_TOO_LONG = 414,
  AZ_HTTP_STATUS_CODE_UNSUPPORTED_MEDIA_TYPE = 415,
  AZ_HTTP_STATUS_CODE_RANGE_NOT_SATISFIABLE = 416,
  AZ_HTTP_STATUS_CODE_EXPECTATION_FAILED = 417,
  AZ_HTTP_STATUS_CODE_MISDIRECTED_REQUEST = 421,
  AZ_HTTP_STATUS_CODE_UNPROCESSABLE_ENTITY = 422,
  AZ_HTTP_STATUS_CODE_LOCKED = 423,
  AZ_HTTP_STATUS_CODE_FAILED_DEPENDENCY = 424,
  AZ_HTTP_STATUS_CODE_TOO_EARLY = 425,
  AZ_HTTP_STATUS_CODE_UPGRADE_REQUIRED = 426,
  AZ_HTTP_STATUS_CODE_PRECONDITION_REQUIRED = 428,
  AZ_HTTP_STATUS_CODE_TOO_MANY_REQUESTS = 429,
  AZ_HTTP_STATUS_CODE_REQUEST_HEADER_FIELDS_TOO_LARGE = 431,
  AZ_HTTP_STATUS_CODE_UNAVAILABLE_FOR_LEGAL_REASONS = 451,

  // 5xx (server error) Status Codes:
  AZ_HTTP_STATUS_CODE_INTERNAL_SERVER_ERROR = 500,
  AZ_HTTP_STATUS_CODE_NOT_IMPLEMENTED = 501,
  AZ_HTTP_STATUS_CODE_BAD_GATEWAY = 502,
  AZ_HTTP_STATUS_CODE_SERVICE_UNAVAILABLE = 503,
  AZ_HTTP_STATUS_CODE_GATEWAY_TIMEOUT = 504,
  AZ_HTTP_STATUS_CODE_HTTP_VERSION_NOT_SUPPORTED = 505,
  AZ_HTTP_STATUS_CODE_VARIANT_ALSO_NEGOTIATES = 506,
  AZ_HTTP_STATUS_CODE_INSUFFICIENT_STORAGE = 507,
  AZ_HTTP_STATUS_CODE_LOOP_DETECTED = 508,
  AZ_HTTP_STATUS_CODE_NOT_EXTENDED = 510,
  AZ_HTTP_STATUS_CODE_NETWORK_AUTHENTICATION_REQUIRED = 511,
} az_http_status_code;

/**
 * An HTTP response status line
 *
 * @see https://tools.ietf.org/html/rfc7230#section-3.1.2
 */
typedef struct
{
  uint8_t major_version;
  uint8_t minor_version;
  az_http_status_code status_code;
  az_span reason_phrase;
} az_http_response_status_line;

/**
 * @brief Initialize az_http_response with a required http_response az_span.
 *
 * The capacity of http_response is the total capacity of the HTTP response that can be written from
 * Network
 *
 */
AZ_NODISCARD AZ_INLINE az_result
az_http_response_init(az_http_response* self, az_span http_response)
{
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
 * @brief Set the az_http_response internal parser to index zero and tries
 * to get status line from it.
 *
 * @param response az_http_response with an http response
 * @param out inline code from http response
 * @return AZ_OK = inline code is parsed and returned.<br>
 * Other value =  http response was not parsed
 */
AZ_NODISCARD az_result
az_http_response_get_status_line(az_http_response* response, az_http_response_status_line* out);

/**
 * @brief parse a header based on the last http response parsed.
 *
 * If called right after parsing status line, this function will try to get the first header from
 * http response.
 *
 * If called right after parsing a header, this function will try to get
 * another header (next one) from http response or will return AZ_ERROR_ITEM_NOT_FOUND if there are
 * no more headers.
 *
 * If called after parsing http body or before parsing status line, this function
 * will return AZ_ERROR_INVALID_STATE
 *
 * @param self an HTTP response
 * @param out an az_pair containing a header when az_result is AZ_OK
 * @return AZ_OK = Header was parsed<br>
 * AZ_ERROR_ITEM_NOT_FOUND = No more headers<br>
 * AZ_ERROR_INVALID_STATE = Can't read a header from current state. Maybe call
 * az_http_response_get_status_line first.
 */
AZ_NODISCARD az_result az_http_response_get_next_header(az_http_response* self, az_pair* out);

/**
 * @brief parses http response body and make out_body point to it.
 *
 * This function can be called directly (no need to call az_http_response_get_status_line and/or
 * az_http_response_get_next_header before). status line and headers are parsed and ignored if this
 * function is called before the others
 *
 * @param self an http response
 * @param out_body out parameter to point to http response body
 * @return AZ_OK = Body parsed and referenced by out_body<br>
 * Other value = Error while trying to read and parse body
 */
AZ_NODISCARD az_result az_http_response_get_body(az_http_response* self, az_span* out_body);

#include <_az_cfg_suffix.h>

#endif // _az_HTTP_H

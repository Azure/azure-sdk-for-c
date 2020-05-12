// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

/**
 * @file az_http.h
 *
 * @brief This header defines the types and functions your application uses
 *        to leverage HTTP request and response functionality.
 *
 * @note You MUST NOT use any symbols (macros, functions, structures, enums, etc.)
 * prefixed with an underscore ('_') directly in your application code. These symbols
 * are part of Azure SDK's internal implementation; we do not document these symbols
 * and they are subject to change in future versions of the SDK which would break your code.
 */

#ifndef _az_HTTP_H
#define _az_HTTP_H

#include <az_config.h>
#include <az_context.h>
#include <az_result.h>
#include <az_span.h>

#include <stdbool.h>
#include <stdint.h>

#include <_az_cfg_prefix.h>

/**
 * @brief The  az_http_status_code enum defines the possible HTTP status codes.
 */
typedef enum
{
  AZ_HTTP_STATUS_CODE_NONE = 0,

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
 * @brief Allows you to customize the retry policy used by SDK clients whenever they performs an I/O
 * operation.
 * @details Client libraries should acquire an initialized instance of this struct and then modify
 * any fields necessary before passing a pointer to this struct when initializing the specific
 * client.
 */
typedef struct
{
  int16_t max_retries;
  int32_t retry_delay_msec;
  int32_t max_retry_delay_msec;
  az_http_status_code const* status_codes;
} az_http_policy_retry_options;

typedef enum
{
  _az_HTTP_RESPONSE_KIND_STATUS_LINE = 0,
  _az_HTTP_RESPONSE_KIND_HEADER = 1,
  _az_HTTP_RESPONSE_KIND_BODY = 2,
  _az_HTTP_RESPONSE_KIND_EOF = 3,
} _az_http_response_kind;

/**
 * @brief Allows you to parse an HTTP response's status line, headers, and body.
 *
 * @details Users create an instance of this and pass it in to an Azure service client's operation
 * function. The function initializes the az_http_response and application code can query the
 * response after the operation completes by calling the #az_http_response_get_status_line,
 * #az_http_response_get_next_header and #az_http_response_get_body functions.
 *
 */
typedef struct
{
  struct
  {
    az_span http_response;
    int32_t written;
    struct
    {
      az_span remaining; // the remaining un-parsed portion of the original http_response.
      _az_http_response_kind next_kind;
      // After parsing an element, next_kind refers to the next expected element
    } parser;
  } _internal;
} az_http_response;

/**
 * @brief az_http_response_init initializes an az_http_response instance over a byte buffer (span)
 * which will be filled with the HTTP response data as it comes in from the network.
 *
 * @param response The pointer to an az_http_response instance which is to be initialized.
 * @param buffer A span over the byte buffer that is to be filled with the HTTP response data. This
 * buffer must be large enough to hold the entire response.
 */
AZ_NODISCARD AZ_INLINE az_result az_http_response_init(az_http_response* response, az_span buffer)
{
  *response = (az_http_response){
    ._internal = {
      .http_response = buffer,
      .written = 0,
      .parser = {
        .remaining = AZ_SPAN_NULL,
        .next_kind = _az_HTTP_RESPONSE_KIND_STATUS_LINE,
      },
    },
  };

  return AZ_OK;
}

/**
 * @brief Represents the result of making an HTTP request.
 * An application obtains this initialized structure by calling #az_http_response_get_status_line.
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
 * @brief az_http_response_get_status_line returns the az_http_response_status_line
 * information within an HTTP response.
 *
 * @param response The az_http_response with an http response.
 * @param out_status_line The pointer to an az_http_response_status_line structure to be filled in
 * by this function.
 * @return AZ_OK = inline code is parsed and returned.<br>
 * Other value =  http response was not parsed
 */
AZ_NODISCARD az_result az_http_response_get_status_line(
    az_http_response* response,
    az_http_response_status_line* out_status_line);

/**
 * @brief az_http_response_get_next_header returns the next HTTP response header.
 *
 * When called right after az_http_response_get_status_line, this function returns the first header.
 * When called after calling az_http_response_get_next_header, this function returns the next
 * header.
 *
 * If called after parsing http body or before parsing status line, this function
 * will return AZ_ERROR_INVALID_STATE
 *
 * @param response A pointer to an az_http_response instance.
 * @param out_header A pointer to an az_pair to receive the header's key & value.
 * @return AZ_OK = A header was returned<br>
 * AZ_ERROR_ITEM_NOT_FOUND = There are no more headers<br>
 * AZ_ERROR_INVALID_STATE = The az_http_response instance is in an invalid state; consider calling
 * az_http_response_get_status_line to reset its state.
 */
AZ_NODISCARD az_result
az_http_response_get_next_header(az_http_response* response, az_pair* out_header);

/**
 * @brief az_http_response_get_body returns a span over the HTTP body within an HTTP response.
 *
 * @param response A pointer to an az_http_response instance.
 * @param out_body A pointer to an az_span to receive the HTTP response's body.
 * @return AZ_OK = An az_span over the reponse body was returned<br>
 * Other value = Error while trying to read and parse body
 */
AZ_NODISCARD az_result az_http_response_get_body(az_http_response* response, az_span* out_body);

#include <_az_cfg_suffix.h>

#endif // _az_HTTP_H

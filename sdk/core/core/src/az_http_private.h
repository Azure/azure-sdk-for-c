// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#ifndef _az_HTTP_PRIVATE_H
#define _az_HTTP_PRIVATE_H

#include <az_http.h>
#include <az_http_transport.h>
#include <az_precondition.h>
#include <az_precondition_internal.h>
#include <az_span.h>

#include <stdbool.h>

#include <_az_cfg_prefix.h>

/**
 * @brief Mark that the HTTP headers that are gong to be added via
 * `az_http_request_append_header` are going to be considered as retry headers.
 *
 * @param p_hrb HTTP request builder.
 *
 * @return
 *   - *`AZ_OK`* success.
 *   - *`AZ_ERROR_ARG`* `p_hrb` is _NULL_.
 */
AZ_NODISCARD AZ_INLINE az_result _az_http_request_mark_retry_headers_start(_az_http_request* p_hrb)
{
  _az_PRECONDITION_NOT_NULL(p_hrb);
  p_hrb->_internal.retry_headers_start_byte_offset
      = p_hrb->_internal.headers_length * (int32_t)sizeof(az_pair);
  return AZ_OK;
}

AZ_NODISCARD AZ_INLINE az_result _az_http_request_remove_retry_headers(_az_http_request* p_hrb)
{
  _az_PRECONDITION_NOT_NULL(p_hrb);
  p_hrb->_internal.headers_length
      = p_hrb->_internal.retry_headers_start_byte_offset / (int32_t)sizeof(az_pair);
  return AZ_OK;
}

/**
 * @brief Sets buffer and parser to its initial state.
 *
 */
void _az_http_response_reset(az_http_response* http_response);

#include <_az_cfg_suffix.h>

#endif // _az_HTTP_PRIVATE_H

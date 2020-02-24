// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#ifndef _az_HTTP_PRIVATE_H
#define _az_HTTP_PRIVATE_H

#include <az_contract_internal.h>
#include <az_http.h>
#include <az_span.h>

#include <stdbool.h>

#include <_az_cfg_prefix.h>

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
AZ_NODISCARD AZ_INLINE az_result _az_http_request_mark_retry_headers_start(_az_http_request* p_hrb)
{
  AZ_CONTRACT_ARG_NOT_NULL(p_hrb);
  p_hrb->_internal.retry_headers_start_byte_offset = az_span_length(p_hrb->_internal.headers);
  return AZ_OK;
}

AZ_NODISCARD AZ_INLINE az_result _az_http_request_remove_retry_headers(_az_http_request* p_hrb)
{
  AZ_CONTRACT_ARG_NOT_NULL(p_hrb);

  az_span* headers_ptr = &p_hrb->_internal.headers;
  *headers_ptr = az_span_init(
      az_span_ptr(*headers_ptr),
      p_hrb->_internal.retry_headers_start_byte_offset,
      az_span_capacity(*headers_ptr));
  return AZ_OK;
}

#include <_az_cfg_suffix.h>

#endif // _az_HTTP_PRIVATE_H

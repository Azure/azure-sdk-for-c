// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

/**
 * @file az_http_transport.h
 *
 * @brief Utilities to be used by transport policy implementations
 */

#ifndef _az_HTTP_TRANSPORT_H
#define _az_HTTP_TRANSPORT_H

#include <az_http.h>

#include <_az_cfg_prefix.h>

AZ_INLINE az_http_method az_http_method_get() { return AZ_SPAN_FROM_STR("GET"); }
AZ_INLINE az_http_method az_http_method_head() { return AZ_SPAN_FROM_STR("HEAD"); }
AZ_INLINE az_http_method az_http_method_post() { return AZ_SPAN_FROM_STR("POST"); }
AZ_INLINE az_http_method az_http_method_put() { return AZ_SPAN_FROM_STR("PUT"); }
AZ_INLINE az_http_method az_http_method_delete() { return AZ_SPAN_FROM_STR("DELETE"); }
AZ_INLINE az_http_method az_http_method_patch() { return AZ_SPAN_FROM_STR("PATCH"); }

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
az_http_request_get_header(_az_http_request* p_request, int32_t index, az_pair* out_result);

AZ_NODISCARD az_result az_http_request_get_parts(
    _az_http_request* p_request,
    az_http_method* out_method,
    az_span* out_url,
    az_span* body);

#include <_az_cfg_suffix.h>

#endif

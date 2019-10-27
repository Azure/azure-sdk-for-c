// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#ifndef AZ_HTTP_HEADER_H
#define AZ_HTTP_HEADER_H

#include <az_span_emitter.h>
#include <az_callback.h>
#include <az_pair.h>

#include <_az_cfg_prefix.h>

/**
 * Emits an HTTP header as a sequence of spans in a format "%{key}: %{value}"
 */
AZ_NODISCARD az_result
az_http_header_emit_spans(az_pair const * p_header, az_span_append const append);

/**
 * @az_http_header_emit_spans callback.
 */
AZ_CALLBACK_FUNC(az_http_header_emit_spans, az_pair const *, az_span_emitter)

#include <_az_cfg_suffix.h>

#endif

// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#ifndef AZ_HTTP_HEADER_H
#define AZ_HTTP_HEADER_H

#include <az_span_seq.h>
#include <az_callback.h>
#include <az_pair.h>

#include <_az_cfg_prefix.h>

/**
 * Converts an HTTP header to a sequence of spans.
 */
AZ_NODISCARD az_result
az_http_header_to_span_seq(az_pair const * p_header, az_span_append const append);

/**
 * @az_http_header_to_span_seq callback.
 */
AZ_CALLBACK_FUNC(az_http_header_to_span_seq, az_pair const *, az_span_seq)

#include <_az_cfg_suffix.h>

#endif

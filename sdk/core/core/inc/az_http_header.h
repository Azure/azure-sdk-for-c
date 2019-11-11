// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#ifndef AZ_HTTP_HEADER_H
#define AZ_HTTP_HEADER_H

#include <az_span_writer.h>
#include <az_pair.h>

#include <_az_cfg_prefix.h>

/**
 * Emits an HTTP header as a sequence of spans in a format "%{key}: %{value}"
 */
AZ_NODISCARD az_result
az_http_header_as_span_writer(az_pair const * const self, az_span_action const write_span);

/**
 * @az_http_header_emit_span_sequence as @az_span_emitter.
 */
AZ_ACTION_FUNC(az_http_header_as_span_writer, az_pair const, az_span_writer)

#include <_az_cfg_suffix.h>

#endif

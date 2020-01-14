// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#ifndef _az_HTTP_HEADER_INTERNAL_H
#define _az_HTTP_HEADER_INTERNAL_H

#include <az_action.h>
#include <az_pair.h>
#include <az_result.h>
#include <az_span.h>
#include <az_span_internal.h>
#include <az_span_writer_internal.h>

#include <_az_cfg_prefix.h>

static az_span const AZ_HTTP_HEADER_CONTENT_LENGTH = AZ_CONST_STR("Content-Length");
static az_span const AZ_HTTP_HEADER_CONTENT_TYPE = AZ_CONST_STR("Content-Type");

static az_span const AZ_HTTP_HEADER_TEXT_PLAIN_CHARSET_UTF_8
    = AZ_CONST_STR("text/plain; charset=UTF-8");

static az_span const AZ_HTTP_HEADER_X_MS_CLIENT_REQUESTID = AZ_CONST_STR("x-ms-client-request-id");
static az_span const AZ_HTTP_HEADER_X_MS_DATE = AZ_CONST_STR("x-ms-date");
static az_span const AZ_HTTP_HEADER_X_MS_VERSION = AZ_CONST_STR("x-ms-version");

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

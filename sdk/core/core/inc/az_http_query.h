// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#ifndef AZ_HTTP_QUERY_H
#define AZ_HTTP_QUERY_H

#include <az_pair.h>

#include <_az_cfg_prefix.h>

/**
 * Emits the given query as a sequence of spans. In a format 
 * "?param1=value1&param2=value2".
 */
AZ_NODISCARD az_result
az_http_query_emit_span_seq(az_pair_emitter const query, az_write_span const write_span);

#include <_az_cfg_suffix.h>

#endif

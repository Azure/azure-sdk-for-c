// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#ifndef AZ_HTTP_QUERY_H
#define AZ_HTTP_QUERY_H

#include "_az_pair.h"
#include "_az_span.h"
#include <az_result.h>
#include <az_span_action.h>

#include <_az_cfg_prefix.h>

/**
 * Emits the given query as a sequence of spans. In a format
 * "?param1=value1&param2=value2".
 */
AZ_NODISCARD az_result
az_http_query_as_span_writer(az_pair_writer const query, az_span_action const write_span);

#include <_az_cfg_suffix.h>

#endif

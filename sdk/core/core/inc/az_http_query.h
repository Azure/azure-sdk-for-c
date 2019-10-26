// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#ifndef AZ_HTTP_QUERY_H
#define AZ_HTTP_QUERY_H

#include <az_pair.h>

#include <_az_cfg_prefix.h>

AZ_NODISCARD az_result
az_http_query_to_span_seq(az_pair_seq const query, az_span_append const append);

#include <_az_cfg_suffix.h>

#endif

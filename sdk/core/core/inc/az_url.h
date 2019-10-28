// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#ifndef AZ_URL_H
#define AZ_URL_H

#include <az_span.h>
#include <az_pair.h>

#include <_az_cfg_prefix.h>

/**
 * https://en.wikipedia.org/wiki/URL
 */
typedef struct {
  az_const_span host;
  az_const_span path;
  az_pair_seq query;
} az_url;

AZ_NODISCARD az_result az_url_emit_span_seq(az_url const * const self, az_span_action const action);

#include <_az_cfg_suffix.h>

#endif

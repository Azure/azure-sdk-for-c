// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#ifndef AZ_HTTP_RESPONSE_ACTION_H
#define AZ_HTTP_RESPONSE_ACTION_H

#include <az_span_emitter.h>
#include <az_pair.h>
#include <az_result.h>
#include <az_span.h>

#include <_az_cfg_prefix.h>

typedef struct {
  az_span_action header_name_part_action;
  az_span_action header_value_part_action;
  az_span_action body_part_action;
} az_http_response_action;

AZ_ACTION_TYPE(az_http_response_emitter, az_http_response_action)

AZ_NODISCARD az_result
az_span_action_to_http_response_action(az_span_action const * const self, az_http_response_action * const out);

AZ_NODISCARD az_result
az_http_response_action_to_span_action(az_http_response_action const * const self, az_span_action * const out);

#include <_az_cfg_suffix.h>

#endif

// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#ifndef _az_LOG_H
#define _az_LOG_H

#include <az_facility.h>
#include <az_span.h>

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#include <_az_cfg_prefix.h>

#define _az_LOG_MAKE_CLASSIFICATION(facility, code) \
  ((int32_t)((uint32_t)(facility) << 16) | (uint32_t)(code))

typedef enum {
  AZ_LOG_ERROR = _az_LOG_MAKE_CLASSIFICATION(AZ_FACILITY_CORE, 1),
  AZ_LOG_REQUEST = _az_LOG_MAKE_CLASSIFICATION(AZ_FACILITY_CORE, 2),
  AZ_LOG_RESPONSE = _az_LOG_MAKE_CLASSIFICATION(AZ_FACILITY_CORE, 3),
  AZ_LOG_RETRY_POLICY = _az_LOG_MAKE_CLASSIFICATION(AZ_FACILITY_CORE, 4),
  AZ_LOG_SLOW_RESPONSE = _az_LOG_MAKE_CLASSIFICATION(AZ_FACILITY_CORE, 5),
} az_log_classification;

typedef struct {
  uint32_t slow_response_threshold_msec; // How long should it take the HTTP request to start being
                                         // considered a slow response (AZ_LOG_SLOW_RESPONSE)
} az_log_options;

typedef void (*az_log)(az_log_classification const classification, az_span const message);

void az_log_set_classifications(
    az_log_classification const * const classifications,
    size_t const classifications_length);

void az_log_set_listener(az_log * const listener);

#include <_az_cfg_suffix.h>

#endif

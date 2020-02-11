// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#ifndef _az_LOG_H
#define _az_LOG_H

#include <az_result.h>
#include <az_span.h>

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#include <_az_cfg_prefix.h>

#define _az_LOG_MAKE_CLASSIFICATION(facility, code) \
  ((int32_t)((uint32_t)(facility) << 16) | (uint32_t)(code))

typedef enum {
  AZ_LOG_HTTP_REQUEST = _az_LOG_MAKE_CLASSIFICATION(AZ_FACILITY_HTTP, 1),
  AZ_LOG_HTTP_RESPONSE = _az_LOG_MAKE_CLASSIFICATION(AZ_FACILITY_HTTP, 2),
} az_log_classification;

typedef void (*az_log_fn)(az_log_classification classification, az_span message);

void az_log_set_classifications(
    az_log_classification const * classifications,
    size_t classifications_length);

void az_log_set_listener(az_log_fn listener);

#include <_az_cfg_suffix.h>

#endif

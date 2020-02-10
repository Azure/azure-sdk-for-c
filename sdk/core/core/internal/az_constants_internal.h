// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#ifndef _az_CONSTANTS_INTERNAL_H
#define _az_CONSTANTS_INTERNAL_H

#include <az_http.h>
#include <az_result.h>
#include <az_span.h>

#include <stdint.h>

#include <_az_cfg_prefix.h>

enum {
  _az_TOKEN_MAX_LENGTH = 2 * 1024,
};

enum {
  _az_TIME_SECONDS_PER_MINUTE = 60,
  _az_TIME_MILLISECONDS_PER_SECOND = 1000,
};

/*
*  Int64 is max value 9223372036854775808   (19 characters)
*           min value -9223372036854775808  (20 characters)
*/
enum {
  _az_INT64_AS_STR_MAX_SIZE = 20,
};

#include <_az_cfg_suffix.h>

#endif /* _az_CONSTANTS_INTERNAL_H */
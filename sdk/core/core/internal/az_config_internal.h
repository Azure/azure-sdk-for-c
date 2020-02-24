// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#ifndef _az_CONFIG_INTERNAL_H
#define _az_CONFIG_INTERNAL_H

#include <az_config.h>
#include <az_span.h>

#include <stdint.h>

#include <_az_cfg_prefix.h>
enum
{
  AZ_HTTP_REQUEST_HEADER_BUF_SIZE = AZ_HTTP_REQUEST_HEADER_COUNT * sizeof(az_pair),
};
/*



*/
enum
{
  _az_TIME_SECONDS_PER_MINUTE = 60,
  _az_TIME_MILLISECONDS_PER_SECOND = 1000,
  _az_TIME_MICROSECONDS_PER_MILLISECOND = 1000,
};

/*
 *  Int64 is max value 9223372036854775808   (19 characters)
 *           min value -9223372036854775808  (20 characters)
 */
enum
{
  _az_INT64_AS_STR_BUF_SIZE = 20,
};

#include <_az_cfg_suffix.h>

#endif // _az_CONFIG_INTERNAL_H

// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#ifndef _az_LOG_INTERNAL_H
#define _az_LOG_INTERNAL_H

#include <az_log.h>
#include <az_span.h>

#include <stdbool.h>

#include <_az_cfg_prefix.h>

// If the user hasn't registered any classifications, then we log everything.

#ifndef AZ_NO_LOGGING
bool _az_log_should_write(az_log_classification classification);
void _az_log_write(az_log_classification classification, az_span message);
#else
AZ_INLINE bool _az_log_should_write(az_log_classification classification)
{
  (void)classification;
  return false;
}

AZ_INLINE void _az_log_write(az_log_classification classification, az_span message)
{
  (void)classification;
  (void)message;
}
#endif // AZ_NO_LOGGING

#include <_az_cfg_suffix.h>

#endif // _az_LOG_INTERNAL_H

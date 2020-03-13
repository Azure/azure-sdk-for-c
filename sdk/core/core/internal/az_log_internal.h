// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#ifndef _az_LOG_INTERNAL_H
#define _az_LOG_INTERNAL_H

#include <az_log.h>
#include <az_span.h>

#include <stdbool.h>

#include <_az_cfg_prefix.h>

// If the user hasn't registered any classifications, then we log everything.
bool az_log_should_write(az_log_classification classification);

void az_log_write(
    az_log_classification classification,
    char const* message,
    int32_t message_length);

AZ_INLINE void az_log_ensure_span_is_str(
    az_span message,
    char const** out_message,
    int32_t* out_message_length)
{
  int32_t length = az_span_length(message);
  int32_t const capacity = az_span_length(message);

  char* const ptr = (char*)az_span_ptr(message);
  *out_message = ptr;

  // We'll need to trim the nessage by 1 character to put 0
  if (length == capacity)
  {
    --length;
  }

  // Having the check below would allow the code to work with read-only buffers that are in the
  // expected format.
  if (ptr[length] != 0)
  {
    ptr[length] = 0;
  }

  *out_message_length = length;
}

#include <_az_cfg_suffix.h>

#endif // _az_LOG_INTERNAL_H

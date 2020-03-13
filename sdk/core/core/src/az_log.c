// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include "az_span_private.h"
#include <az_config.h>
#include <az_http.h>
#include <az_http_internal.h>
#include <az_http_transport.h>
#include <az_log.h>
#include <az_log_internal.h>
#include <az_span.h>

#include <stddef.h>

#include <_az_cfg.h>

static az_log_classification const* _az_log_classifications = NULL;
static az_log_message_fn _az_log_message_callback = NULL;

void az_log_set_classifications(az_log_classification const classifications[])
{
  _az_log_classifications = classifications;
}

void az_log_set_callback(az_log_message_fn az_log_message_callback)
{
  _az_log_message_callback = az_log_message_callback;
}

void az_log_write(az_log_classification classification, az_span message)
{
  if (_az_log_message_callback != NULL && az_log_should_write(classification))
  {
    int32_t const length = az_span_length(message);
    int32_t const capacity = az_span_capacity(message);
    // Do nothing if span is empty or if truncating it with zero would result in empty span.
    if (length > 0 && capacity > 1)
    {
      int32_t const z_pos = length < capacity ? length + 1 : capacity - 1;
      char* const buf = (char*)az_span_ptr(message);

      // Don't write 0 if it's already there, for instance when the code logs a string literal span,
      // don't try to modify it as there is a chance it resides in .text.
      if (buf[z_pos] != 0)
      {
        buf[z_pos] = 0;
      }

      _az_log_message_callback(classification, buf, z_pos - 1);
    }
  }
}

bool az_log_should_write(az_log_classification classification)
{
  if (_az_log_message_callback == NULL)
  {
    // If no one is listening, don't attempt to log.
    return false;
  }

  if (_az_log_classifications == NULL)
  {
    // If the user hasn't registered any classifications, then we log everything.
    return true;
  }

  for (az_log_classification const* cls = _az_log_classifications; *cls != AZ_LOG_END_OF_LIST;
       ++cls)
  {
    // Return true if a classification is in the customer-provided list.
    if (*cls == classification)
    {
      return true;
    }
  }

  // Classification is not in the customer-provided list - return false.
  return false;
}

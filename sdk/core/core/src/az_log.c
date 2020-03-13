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

void az_log_write(az_log_classification classification, char const* message, int32_t message_length)
{
  if (_az_log_message_callback != NULL && az_log_should_write(classification))
  {
    _az_log_message_callback(classification, message, message_length);
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

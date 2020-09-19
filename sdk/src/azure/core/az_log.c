// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include "az_span_private.h"
#include <azure/core/az_config.h>
#include <azure/core/az_http.h>
#include <azure/core/az_http_transport.h>
#include <azure/core/az_log.h>
#include <azure/core/az_span.h>
#include <azure/core/internal/az_http_internal.h>
#include <azure/core/internal/az_log_internal.h>

#include <stddef.h>

#include <azure/core/_az_cfg.h>

#ifndef AZ_NO_LOGGING

static az_log_classification volatile _az_log_classifications = AZ_LOG_NONE;
static az_log_message_fn volatile _az_log_message_callback = NULL;

void az_log_set_classifications(az_log_classification const classifications)
{
  _az_log_classifications = classifications;
}

void az_log_set_callback(az_log_message_fn az_log_message_callback)
{
  _az_log_message_callback = az_log_message_callback;
}

// This function returns whether or not the passed-in message should be logged.
bool _az_log_should_write(az_log_classification classification)
{
  _az_PRECONDITION(classification > 0);

  // Copy the volatile fields to local variables so that they don't change within this function
  az_log_message_fn const callback = _az_log_message_callback;
  az_log_classification const bit_flag_of_classifications = _az_log_classifications;

  // If no one is listening, don't attempt to log.
  // Otherwise, log if the bit-value corresponding to the particular classification is 1.
  return (callback != NULL)
      && (bit_flag_of_classifications == AZ_LOG_ALL
          || ((bit_flag_of_classifications & classification) != 0));
}

// This function attempts to log the passed-in message.
void _az_log_write(az_log_classification classification, az_span message)
{
  _az_PRECONDITION(classification > 0);
  _az_PRECONDITION_VALID_SPAN(message, 0, true);

  // Copy the volatile fields to local variables so that they don't change within this function
  az_log_message_fn const callback = _az_log_message_callback;

  // If no one is listening, don't attempt to log.
  // Otherwise, log if the bit-value corresponding to the particular classification is 1.
  if (_az_log_should_write(classification))
  {
    callback(classification, message);
  }
}

#endif // AZ_NO_LOGGING

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

static az_log_message_fn volatile _az_log_message_callback = NULL;
static az_log_should_write_fn volatile _az_log_should_write_callback = NULL;

void az_log_set_callbacks(
    az_log_message_fn az_log_message_callback,
    az_log_should_write_fn az_log_should_write_callback)
{
  _az_log_message_callback = az_log_message_callback;
  _az_log_should_write_callback = az_log_should_write_callback;
}

// This function returns whether or not the passed-in message should be logged.
bool _az_log_should_write(az_log_classification classification)
{
  _az_PRECONDITION(classification > 0);

  // Copy the volatile fields to local variables so that they don't change within this function
  az_log_message_fn const message_callback = _az_log_message_callback;
  az_log_should_write_fn const should_write_callback = _az_log_should_write_callback;

  // If the user hasn't registered a should_write_callback, then we log everything, as long as a
  // message_callback metho was provided.
  return (should_write_callback == NULL || should_write_callback(classification))
      && message_callback != NULL;
}

// This function attempts to log the passed-in message.
void _az_log_write(az_log_classification classification, az_span message)
{
  _az_PRECONDITION(classification > 0);
  _az_PRECONDITION_VALID_SPAN(message, 0, true);

  // Copy the volatile fields to local variables so that they don't change within this function
  az_log_message_fn const message_callback = _az_log_message_callback;

  // If the user hasn't registered a should_write_callback, then we log everything, as long as a
  // message_callback metho was provided.
  if (_az_log_should_write(classification))
  {
    message_callback(classification, message);
  }
}

#endif // AZ_NO_LOGGING

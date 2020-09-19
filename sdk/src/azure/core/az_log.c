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

// Only using volatile here, not for thread safety, but so that the compiler does not optimize what
// it thinks falsely thinks are stale reads.
static az_log_message_fn volatile _az_log_message_callback = NULL;

void az_log_set_callback(az_log_message_fn az_log_message_callback)
{
  _az_log_message_callback = az_log_message_callback;
}

// This function attempts to log the passed-in message.
void _az_log_write(az_log_classification classification, az_span message)
{
  _az_PRECONDITION(classification > 0);
  _az_PRECONDITION_VALID_SPAN(message, 0, true);

  // Copy the volatile fields to local variables so that they don't change within this function.
  az_log_message_fn const message_callback = _az_log_message_callback;

  if (message_callback != NULL)
  {
    message_callback(classification, message);
  }
}

#endif // AZ_NO_LOGGING

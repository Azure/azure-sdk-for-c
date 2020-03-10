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

#include <stdbool.h>
#include <stddef.h>

#include <_az_cfg.h>

static az_log_classification const* _az_log_classifications = NULL;
static size_t _az_log_classifications_length = 0;
static az_log_fn _az_log_listener = NULL;

void az_log_set_classifications(
    az_log_classification const* classifications,
    size_t classifications_length)
{
  // TODO: thread safety
  _az_log_classifications = classifications;
  _az_log_classifications_length = classifications_length;
}

void az_log_set_listener(az_log_fn listener)
{
  // TODO: thread safety
  _az_log_listener = listener;
}

void az_log_write(az_log_classification classification, az_span message)
{
  // TODO: thread safety
  if (_az_log_listener != NULL && az_log_should_write(classification))
  {
    (*_az_log_listener)(classification, message);
  }
}

bool az_log_should_write(az_log_classification classification)
{
  // TODO: thread safety
  if (_az_log_listener == NULL)
  {
    // If no one is listening, don't attempt to log.
    return false;
  }
  if (_az_log_classifications == NULL || _az_log_classifications_length == 0)
  {
    // If the user hasn't registered any classifications, then we log everything.
    return true;
  }

  for (size_t i = 0; i < _az_log_classifications_length; ++i)
  {
    // Return true if a classification is in the customer-provided whitelist.
    if (_az_log_classifications[i] == classification)
    {
      return true;
    }
  }

  // Classification is not in the whitelist - return false.
  return false;
}

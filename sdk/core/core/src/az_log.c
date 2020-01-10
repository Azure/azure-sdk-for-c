// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <az_log.h>

#include <_az_cfg.h>

static az_log_classification const * _az_log_classifications = NULL;
static size_t _az_log_classifications_length = 0;
static az_log * _az_log_listener = NULL;

void az_log_set_classifications(
    az_log_classification const * const classifications,
    size_t const classifications_length) {
  _az_log_classifications = classifications;
  _az_log_classifications_length = classifications_length;
}

void az_log_reset_classifications() {
  _az_log_classifications = NULL;
  _az_log_classifications_length = 0;
}

void az_log_set_listener(az_log * const listener) { _az_log_listener = listener; }

void az_log_write(az_log_classification const classification, char const * const message) {
  if (_az_log_listener != NULL && az_log_should_write(classification)) {
    (*_az_log_listener)(classification, message);
  }
}

bool az_log_should_write(az_log_classification const classification) {
  if (_az_log_classifications == NULL || _az_log_classifications_length == 0) {
    return true;
  }

  for (size_t i = 0; i < _az_log_classifications_length; ++i) {
    if (_az_log_classifications[i] == classification) {
      return true;
    }
  }

  return false;
}

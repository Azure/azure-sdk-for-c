// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#ifndef _az_LOG_INTERNAL_H
#define _az_LOG_INTERNAL_H

#include <az_log.h>
#include <az_span.h>

#include <_az_cfg_prefix.h>

void az_log_write(az_log_classification const classification, az_span const message);

// If the user hasn't registered any classifications, then we log everything.
bool az_log_should_write(az_log_classification const classification);

#include <_az_cfg_suffix.h>

#endif

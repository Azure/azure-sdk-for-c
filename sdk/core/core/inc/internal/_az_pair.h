// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#ifndef _AZ_PAIR_H
#define _AZ_PAIR_H

#include "az_action.h"
#include <az_pair.h>
#include <az_result.h>
#include <az_span.h>

#include <stdbool.h>
#include <stddef.h>

#include <_az_cfg_prefix.h>

/// @az_pair_action is a callback with one argument @az_pair.
AZ_ACTION_TYPE(az_pair_action, az_pair)

/// @var az_pair_writer writes @var az_pair sequences.
AZ_ACTION_TYPE(az_pair_writer, az_pair_action)

AZ_NODISCARD az_result
az_pair_span_as_writer(az_pair_span const * const self, az_pair_action const write_pair);

/**
 * Creates @var az_pair_span_writer from @az_write_pair_span.
 */
AZ_ACTION_FUNC(az_pair_span_as_writer, az_pair_span const, az_pair_writer)

#include <_az_cfg_suffix.h>

#endif

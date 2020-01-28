// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#ifndef _az_SPAN_ACTION_H
#define _az_SPAN_ACTION_H

#include <az_action.h>
#include <az_span.h>

#include <_az_cfg_prefix.h>

/**
 * ```c
 * typedef struct {
 *   az_result (* func)(void *, az_const_span);
 *   void * self;
 * } az_span_action;
 * ```
 *
 * Example of usage
 *
 * ```c
 * az_span_action const action = ...;
 * az_span_action_do(action, AZ_SPAN_FROM_STR("Something"));
 * ```
 */
AZ_ACTION_TYPE(az_span_action, az_span)

#include <_az_cfg_suffix.h>

#endif

// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#ifndef AZ_SPAN_ACTION_H
#define AZ_SPAN_ACTION_H

#include <az_action.h>

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
 * az_span_action_do(action, AZ_STR("Something"));
 * ```
 */
AZ_ACTION_TYPE(az_span_action, az_span)

#include <_az_cfg_suffix.h>

#endif

// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#ifndef _az_INTERNAL_MUT_SPAN_H
#define _az_INTERNAL_MUT_SPAN_H

#include <az_action.h>
#include <az_contract.h>
#include <az_mut_span.h>
#include <az_result.h>

#include <stdbool.h>

#include <_az_cfg_prefix.h>

AZ_NODISCARD AZ_INLINE bool az_mut_span_is_valid(az_mut_span const span) {
  return span.size == 0 || (span.begin != NULL && span.begin <= span.begin + span.size - 1);
}

#define AZ_CONTRACT_ARG_VALID_MUT_SPAN(span) AZ_CONTRACT(az_mut_span_is_valid(span), AZ_ERROR_ARG)

/**
 * ```c
 * typedef struct {
 *   az_result (* func)(void *, az_mut_span);
 *   void * self;
 * } az_mut_span_action;
 * ```
 *
 * Example of usage
 *
 * ```c
 * az_mut_span const span = ...;
 * az_mut_span_action const action = ...;
 * az_mut_span_action_do(action, span);
 * ```
 */
AZ_ACTION_TYPE(az_mut_span_action, az_mut_span)

#include <_az_cfg_suffix.h>

#endif

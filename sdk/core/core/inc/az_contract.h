// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#ifndef _az_CONTRACT_H
#define _az_CONTRACT_H

#include <az_result.h>

#include <_az_cfg_extern_include_prefix.h>

#include <stddef.h>

#include <_az_cfg_extern_include_suffix.h>

#include <_az_cfg_prefix.h>

#define AZ_CONTRACT(condition, error) \
  do { \
    if (!(condition)) { \
      return error; \
    } \
  } while (0)

#define AZ_CONTRACT_ARG_NOT_NULL(arg) AZ_CONTRACT((arg) != NULL, AZ_ERROR_ARG)

#define AZ_CONTRACT_ARG_VALID_SPAN(span) AZ_CONTRACT(az_span_is_valid(span), AZ_ERROR_ARG)
#define AZ_CONTRACT_ARG_VALID_MUT_SPAN(span) AZ_CONTRACT(az_mut_span_is_valid(span), AZ_ERROR_ARG)

#include <_az_cfg_suffix.h>

#endif

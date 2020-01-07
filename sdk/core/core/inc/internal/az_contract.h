// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#ifndef AZ_CONTRACT_H
#define AZ_CONTRACT_H

#include <az_result.h>

#include <stddef.h>

#include <_az_cfg_prefix.h>

#define AZ_CONTRACT(condition, error) \
  do { \
    if (!(condition)) { \
      return error; \
    } \
  } while (0)

#define AZ_CONTRACT_ARG_NOT_NULL(arg) AZ_CONTRACT((arg) != NULL, AZ_ERROR_ARG)

#include <_az_cfg_suffix.h>

#endif

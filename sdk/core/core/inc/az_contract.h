// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#ifndef AZ_ITER_H
#define AZ_ITER_H

#include <az_span.h>

#include <_az_cfg_prefix.h>

#define AZ_CONTRACT_ARG_NOT_NULL(arg) \
  do { \
    if ((arg) == NULL) { \
      return AZ_ERROR_ARG; \
    } \
  } while (0)

#include <_az_cfg_suffix.h>

#endif

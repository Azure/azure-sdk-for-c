// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#ifndef AZ_STATIC_ASSERT_H
#define AZ_STATIC_ASSERT_H

#include <stdbool.h>

#include <_az_cfg_prefix.h>

#define AZ_CAT(A, B) A##B

#define AZ_STATIC_ASSERT(CONDITION, NAME) \
  int AZ_CAT(az_static_assert_, NAME)(int x[(CONDITION) ? 1 : -1]);

AZ_STATIC_ASSERT(true, TRUE)

#include <_az_cfg_suffix.h>

#endif

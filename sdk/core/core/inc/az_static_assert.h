// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#ifndef _az_STATIC_ASSERT_H
#define _az_STATIC_ASSERT_H

#include <_az_cfg_extern_include_prefix.h>

#include <stdbool.h>

#include <_az_cfg_extern_include_suffix.h>

#include <_az_cfg_prefix.h>

#define AZ_STATIC_ASSERT(CONDITION) int az_static_assert(int x[(CONDITION) ? 1 : -1]);

AZ_STATIC_ASSERT(true)

#include <_az_cfg_suffix.h>

#endif

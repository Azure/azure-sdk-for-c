// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#ifndef _az_PLATFORM_INTERNAL_H
#define _az_PLATFORM_INTERNAL_H

#include <az_result.h>

#include <stdint.h>

#include <_az_cfg_prefix.h>

AZ_NODISCARD int64_t az_platform_clock_msec();

void az_platform_sleep_msec(int32_t milliseconds);

#include <_az_cfg_suffix.h>

#endif // _az_PLATFORM_INTERNAL_H

// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#ifndef _az_PLATFORM_INTERNAL_H
#define _az_PLATFORM_INTERNAL_H

#include <az_platform_impl.h>
#include <az_result.h>

#include <stdint.h>

#include <_az_cfg_prefix.h>

AZ_NODISCARD int64_t az_platform_clock_msec();

void az_platform_sleep_msec(int32_t milliseconds);

typedef struct az_platform_mtx az_platform_mtx;

void az_platform_mtx_destroy(az_platform_mtx* mtx);
AZ_NODISCARD az_result az_platform_mtx_init(az_platform_mtx* mtx);
AZ_NODISCARD az_result az_platform_mtx_lock(az_platform_mtx* mtx);
AZ_NODISCARD az_result az_platform_mtx_unlock(az_platform_mtx* mtx);

#include <_az_cfg_suffix.h>

#endif

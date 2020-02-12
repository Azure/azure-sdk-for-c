// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <az_platform.h>

#include <threads.h>
#include <time.h>

#include <_az_cfg.h>

AZ_NODISCARD int64_t az_platform_clock_msec() {
  // Convert clock_t to milliseconds
  // Floating point arithmetic is used to cover CLOCKS_PER_SEC all values
  // including 12000, 2000, 1500, 300, 500, 100, 2
  return (int64_t)(clock() / (CLOCKS_PER_SEC / 1000.0));
}

void az_platform_sleep_msec(int32_t milliseconds) {
  
}

typedef struct az_platform_mtx az_platform_mtx;

void az_platform_mtx_destroy(az_platform_mtx * mtx);
AZ_NODISCARD az_result az_platform_mtx_init(az_platform_mtx * mtx);
AZ_NODISCARD az_result az_platform_mtx_lock(az_platform_mtx * mtx);
AZ_NODISCARD az_result az_platform_mtx_unlock(az_platform_mtx * mtx);
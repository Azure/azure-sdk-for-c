// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#ifndef _az_PLATFORM_IMPL_H
#define _az_PLATFORM_IMPL_H

#ifndef _az_MOCK_ENABLED
#error "Mock platform is only compatible with _az_MOCK_ENABLED"
#endif

#include <azure/core/az_result.h>

#include <stdbool.h>
#include <stdint.h>

#include <azure/core/_az_cfg.h>

#include <cmocka.h>
//Perhaps drop the inline for the mocks implementation
//  Alternative, go back to linking in the platform

/**
 * @brief Platform implementation for mock testing
 * @return Value stored in g_az_platform_clock_value.
 */
AZ_NODISCARD AZ_INLINE int64_t az_platform_clock_msec()
{
  return (int64_t)mock();
}

/**
 * @brief Platform implementation for mock testing
 * @param milliseconds Number of milliseconds to sleep.
 * @remarks Non-positive values (milliseconds <= 0) have undefined behavior
 */
AZ_INLINE void az_platform_sleep_msec(int32_t milliseconds)
{
  return 0;
}

/**
 * @brief Platform implementation for mock testing
 *
 * @param obj A pointer to a pointer that needs to be changed.
 * @param expected An expected value of a value that \p obj is pointing to, prior to exchange.
 * @param desired A value to assign to the value that is pointed by \p obj, if its value is equal to
 * \p expected.
 *
 * @return `true` if previous value of \p obj matches the \p expected.
 *
 * @remarks If `*obj` equals to \p expected, its value is being overwritten with \p desired, and no
 * operation otherwise. Returns `true` if the value was overwritten, `false` otherwise.
 */
AZ_NODISCARD AZ_INLINE bool az_platform_atomic_compare_exchange(
    uintptr_t volatile* obj,
    uintptr_t expected,
    uintptr_t desired)
{
  return false;
}

#endif //__az_PLATFORM_IMPL_H

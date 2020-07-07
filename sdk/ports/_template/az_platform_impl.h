// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#ifndef _az_PLATFORM_IMPL_H
#define _az_PLATFORM_IMPL_H

#include <azure/core/az_result.h>

#include <stdbool.h>
#include <stdint.h>

#include <azure/core/_az_cfg.h>

/**
 * @brief Gets the platform clock in milliseconds.
 * @return Platform clock in milliseconds.
 */
AZ_NODISCARD AZ_INLINE int64_t az_platform_clock_msec()
{
// The method must be implemented
// The Azure SDK for Embedded C provides default implementations for Win32, Linux and Mac.
//  https://github.com/Azure/azure-sdk-for-c#cmake-options
#error \
    "No method implementation provided, see documentation for additional details.  https://github.com/Azure/azure-sdk-for-c"
  return 0;
}

/**
 * @brief Tells the platform to sleep for a given number of milliseconds.
 * @param milliseconds Number of milliseconds to sleep.
 * @remarks Non-positive values (milliseconds <= 0) have undefined behavior
 */
AZ_INLINE void az_platform_sleep_msec(int32_t milliseconds)
{
// The method must be implemented
// The Azure SDK for Embedded C provides default implementations for Win32, Linux and Mac.
//  https://github.com/Azure/azure-sdk-for-c#cmake-options
#error \
    "No method implementation provided, see documentation for additional details.  https://github.com/Azure/azure-sdk-for-c"
  return;
}

/**
 * @brief Conditionally exchanges values of two pointers in a
 * thread-safe manner.
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
// The method must be implemented
// The Azure SDK for Embedded C provides default implementations for Win32, Linux and Mac.
//  https://github.com/Azure/azure-sdk-for-c#cmake-options
#error \
    "No method implementation provided, see documentation for additional details.  https://github.com/Azure/azure-sdk-for-c"
  return false;
}

#endif //__az_PLATFORM_IMPL_H

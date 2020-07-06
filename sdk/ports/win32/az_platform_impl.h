// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#ifndef _az_PLATFORM_IMPL_H
#define _az_PLATFORM_IMPL_H

// Two macros below are not used in the code below, it is windows.h that consumes them.
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>

#include <azure/core/_az_cfg.h>

/**
 * @brief Gets the platform clock in milliseconds.
 * @return Platform clock in milliseconds.
 */
inline AZ_NODISCARD int64_t az_platform_clock_msec() { return GetTickCount64(); }

/**
 * @brief Tells the platform to sleep for a given number of milliseconds.
 * @param milliseconds Number of milliseconds to sleep.
 *        0 - Yield remainder of time slice and resume immediately
 *       -1 - Sleep should not time out.
 * @remarks Negative values (excluding -1) have undefined behavior
 */
inline void az_platform_sleep_msec(int32_t milliseconds) { Sleep(milliseconds); }

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
inline AZ_NODISCARD bool az_platform_atomic_compare_exchange(
    uintptr_t volatile* obj,
    uintptr_t expected,
    uintptr_t desired)
{
  return InterlockedCompareExchangePointer((void* volatile*)obj, (void*)desired, (void*)expected)
      == (void*)expected;
}

#endif //_az_PLATFORM_IMPL_H

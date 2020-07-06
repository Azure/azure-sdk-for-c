// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#ifndef _az_PLATFORM_IMPL_H
#define _az_PLATFORM_IMPL_H

#include <azure/core/internal/az_config_internal.h>
#include <time.h>
#include <unistd.h>
#include <azure/core/_az_cfg.h>

/**
 * @brief Gets the platform clock in milliseconds.
 * @return Platform clock in milliseconds.
 */
inline AZ_NODISCARD int64_t az_platform_clock_msec()
{
  return (int64_t)((clock() / CLOCKS_PER_SEC) * _az_TIME_MILLISECONDS_PER_SECOND);
}

/**
 * @brief Tells the platform to sleep for a given number of milliseconds.
 * @param milliseconds Number of milliseconds to sleep.
 *        0 - Yield remainder of time slice and resume immediately
 *       -1 - Sleep should not time out.
 * @remarks Negative values (excluding -1) have undefined behavior
 */
inline void az_platform_sleep_msec(int32_t milliseconds)
{
  (void)usleep((useconds_t)milliseconds * _az_TIME_MICROSECONDS_PER_MILLISECOND);
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
inline AZ_NODISCARD bool az_platform_atomic_compare_exchange(
    uintptr_t volatile* obj,
    uintptr_t expected,
    uintptr_t desired)
{
#if defined(__GNUC__) && (__GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 1))
  // __GNUC__ being defined does not mean that the compiler being used is necessarily GCC, but it
  // means that the compiler supports GNU extensions to the C language. Clang supports it, Intel
  // likely does. __sync_bool_compare_and_swap is one of these extensions, supported since the
  // version 4.1.
  // https://gcc.gnu.org/onlinedocs/gcc-4.1.0/gcc/C-Extensions.html
  // https://gcc.gnu.org/onlinedocs/gcc-4.1.0/gcc/Atomic-Builtins.html#Atomic-Builtins

  return __sync_bool_compare_and_swap(obj, expected, desired);
#else
  return AZ_ERROR_NOT_IMPLEMENTED;
#endif // __GNUC__ >= 4.1
}

#endif //__az_PLATFORM_IMPL_H

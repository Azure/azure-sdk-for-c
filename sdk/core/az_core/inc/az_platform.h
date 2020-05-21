// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#ifndef _az_PLATFORM_H
#define _az_PLATFORM_H

#include <az_result.h>

#include <stdbool.h>
#include <stdint.h>

#include <_az_cfg_prefix.h>

/**
 * @brief Gets the platform clock in milliseconds.
 * @remark The moment of time where clock starts is undefined, but if this function is getting
 * called twice with one second interval, the difference between the values returned should be equal
 * to 1000.
 * @return Platform clock in milliseconds.
 */
AZ_NODISCARD int64_t az_platform_clock_msec();

/**
 * @brief Tells the platform to sleep for a given number of milliseconds.
 * @param milliseconds Number of milliseconds to sleep.
 * @remarks The behavior is undefined when \p milliseconds is a non-positive value (0 or less than
 * 0).
 */
void az_platform_sleep_msec(int32_t milliseconds);

/**
 * @brief Conditionally exchanges values of two pointers in a
 * thread-safe manner.
 *
 * @param obj A pointer to a pointer that needs to be changed.
 * @param expected An expected value of a value that \p obj is pointing to, prior to exchange.
 * @param desired A value to assign to the value that is pointed by \p obj, if its value equals to
 * \p expected.
 *
 * @return `true` if previous value of \p obj matches the \p expected.
 *
 * @remarks If `*obj` equals to \p expected, its value is being overwritten with \p desired, and no
 * operation otherwise. Returns `true` if the value was overwritten, `false` otherwise.
 */
AZ_NODISCARD bool az_platform_atomic_compare_exchange(
    uintptr_t volatile* obj,
    uintptr_t expected,
    uintptr_t desired);

#include <_az_cfg_suffix.h>

#endif // _az_PLATFORM_H

// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

/**
 * @file
 *
 * @brief Defines platform-specific functionality used by the Azure SDK.
 *
 * @note You MUST NOT use any symbols (macros, functions, structures, enums, etc.)
 * prefixed with an underscore ('_') directly in your application code. These symbols
 * are part of Azure SDK's internal implementation; we do not document these symbols
 * and they are subject to change in future versions of the SDK which would break your code.
 */

#ifndef _az_PLATFORM_H
#define _az_PLATFORM_H

#include <azure/core/az_result.h>

#include <stdbool.h>
#include <stdint.h>

#include <azure/core/_az_cfg_prefix.h>

/**
 * @brief Gets the platform clock in milliseconds.
 *
 * @remark The moment of time where clock starts is undefined, but if this function is getting
 * called twice with one second interval, the difference between the values returned should be equal
 * to 1000.
 *
 * @return Platform clock in milliseconds.
 */
AZ_NODISCARD int64_t az_platform_clock_msec();

/**
 * @brief Tells the platform to sleep for a given number of milliseconds.
 *
 * @param[in] milliseconds Number of milliseconds to sleep.
 *
 * @remarks The behavior is undefined when \p milliseconds is a non-positive value (0 or less than
 * 0).
 */
void az_platform_sleep_msec(int32_t milliseconds);

/**
 * @brief Conditionally exchanges values of two pointers in a thread-safe manner.
 *
 * @param[in,out] ref_obj A pointer to a `volatile` pointer that needs to be changed.
 * @param[in] expected An expected value of a value that \p obj is pointing to, prior to exchange.
 * @param[in] desired A value to assign to the value that is pointed by \p obj, if its value equals
 * to \p expected.
 *
 * @return `true` if previous value of \p obj matches the \p expected.
 *
 * @remarks If `*obj` equals to \p expected, its value is being overwritten with \p desired, and no
 * operation otherwise. Returns `true` if the value was overwritten, `false` otherwise.
 */
AZ_NODISCARD bool az_platform_atomic_compare_exchange(
    uintptr_t volatile* ref_obj,
    uintptr_t expected,
    uintptr_t desired);

#include <azure/core/_az_cfg_suffix.h>

#endif // _az_PLATFORM_H

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
#include <azure/core/az_span.h>
#ifndef __APPLE__
#include <azure/platform/internal/az_platform_internal.h>
#endif
#include <stdbool.h>
#include <stdint.h>
#if defined(PLATFORM_POSIX)
#include "azure/platform/az_platform_posix.h"
#elif defined(PLATFORM_WIN32)
#include "azure/platform/az_platform_win32.h"
#else
#include "azure/platform/az_platform_none.h"
#endif

#include <azure/core/_az_cfg_prefix.h>

/**
 * @brief Gets the platform clock in milliseconds.
 *
 * @remark The moment of time where clock starts is undefined, but if this function is getting
 * called twice with one second interval, the difference between the values returned should be equal
 * to 1000.
 *
 * @param[out] out_clock_msec Platform clock in milliseconds.
 *
 * @return An #az_result value indicating the result of the operation.
 * @retval #AZ_OK Success.
 * @retval #AZ_ERROR_DEPENDENCY_NOT_PROVIDED No platform implementation was supplied to support this
 * function.
 */
AZ_NODISCARD az_result az_platform_clock_msec(int64_t* out_clock_msec);

/**
 * @brief Tells the platform to sleep for a given number of milliseconds.
 *
 * @param[in] milliseconds Number of milliseconds to sleep.
 *
 * @remarks The behavior is undefined when \p milliseconds is a non-positive value (0 or less than
 * 0).
 *
 * @return An #az_result value indicating the result of the operation.
 * @retval #AZ_OK Success.
 * @retval #AZ_ERROR_DEPENDENCY_NOT_PROVIDED No platform implementation was supplied to support this
 * function.
 */
AZ_NODISCARD az_result az_platform_sleep_msec(int32_t milliseconds);

/**
 * @brief Gets a positive pseudo-random integer.
 *
 * @param[out] out_random A pseudo-random number greater than or equal to 0.
 *
 * @retval #AZ_OK Success.
 * @retval #AZ_ERROR_DEPENDENCY_NOT_PROVIDED No platform implementation was supplied to support this
 * function.
 *
 * @note This is NOT cryptographically secure.
 */
AZ_NODISCARD az_result az_platform_get_random(int32_t* out_random);

#ifndef __APPLE__

/**
 * @brief Called on critical error. This function should not return.
 *
 * @note Must be defined by the application.
 *
 * @details In general, this function should cause the device to reboot or the main process to
 *          crash or exit.
 */
void az_platform_critical_error();

/**
 * @brief Create a timer object.
 *
 * @param[out] out_timer The timer handle.
 * @param[in] callback SDK callback to call when timer elapses.
 * @param[in] callback_context SDK data associated with the timer.
 *
 * @return An #az_result value indicating the result of the operation.
 * @retval #AZ_OK Success.
 * @retval #AZ_ERROR_DEPENDENCY_NOT_PROVIDED No platform implementation was supplied to support this
 * function.
 * @retval #AZ_ERROR_OUT_OF_MEMORY Out of memory, unable to allocate timer.
 * @retval #AZ_ERROR_ARG Invalid argument.
 */
AZ_NODISCARD az_result az_platform_timer_create(
    _az_platform_timer* out_timer,
    _az_platform_timer_callback callback,
    void* callback_context);

/**
 * @brief Starts the timer. This function can be called multiple times. The timer should call the
 *        callback at most once.
 *
 * @param[out] out_timer The timer handle.
 * @param[in] milliseconds Time in milliseconds after which the platform must call the associated
 *                     _az_platform_timer_callback().
 *
 * @return An #az_result value indicating the result of the operation.
 * @retval #AZ_OK Success.
 * @retval #AZ_ERROR_DEPENDENCY_NOT_PROVIDED No platform implementation was supplied to support this
 * function.
 * @retval #AZ_ERROR_ARG Invalid milliseconds.
 */
AZ_NODISCARD az_result az_platform_timer_start(_az_platform_timer* out_timer, int32_t milliseconds);

/**
 * @brief Destroys a timer.
 *
 * @param[out] out_timer The timer handle.
 *
 * @return An #az_result value indicating the result of the operation.
 * @retval #AZ_OK Success.
 * @retval #AZ_ERROR_DEPENDENCY_NOT_PROVIDED No platform implementation was supplied to support this
 * function.
 * @retval #AZ_ERROR_ARG Invalid timer provided.
 */
AZ_NODISCARD az_result az_platform_timer_destroy(_az_platform_timer* out_timer);

/**
 * @brief Initializes a mutex, the mutex must be reentrant.
 *
 * @param[in] mutex_handle The mutex handle.
 *
 * @return An #az_result value indicating the result of the operation.
 * @retval #AZ_OK Success.
 * @retval #AZ_ERROR_DEPENDENCY_NOT_PROVIDED No platform implementation was supplied to support this
 * function.
 * @retval #AZ_ERROR_OUT_OF_MEMORY Out of memory, unable to initialize mutex.
 * @retval #AZ_ERROR_ARG Invalid argument.
 */
AZ_NODISCARD az_result az_platform_mutex_init(az_platform_mutex* mutex_handle);

/**
 * @brief Acquires a mutex.
 *
 * @param[in] mutex_handle The mutex handle.
 *
 * @return An #az_result value indicating the result of the operation.
 * @retval #AZ_OK success.
 * @retval #AZ_ERROR_DEPENDENCY_NOT_PROVIDED No platform implementation was supplied to support this
 * function.
 * @retval #AZ_ERROR_ARG Invalid argument.
 */
AZ_NODISCARD az_result az_platform_mutex_acquire(az_platform_mutex* mutex_handle);

/**
 * @brief Releases a mutex.
 *
 * @param[in] mutex_handle The mutex handle.
 *
 * @return An #az_result value indicating the result of the operation.
 * @retval #AZ_OK success.
 * @retval #AZ_ERROR_DEPENDENCY_NOT_PROVIDED No platform implementation was supplied to support this
 * function.
 * @retval #AZ_ERROR_ARG Invalid argument.
 */
AZ_NODISCARD az_result az_platform_mutex_release(az_platform_mutex* mutex_handle);

/**
 * @brief Destroys a mutex.
 *
 * @param[in] mutex_handle The mutex handle.
 *
 * @return An #az_result value indicating the result of the operation.
 * @retval #AZ_OK success.
 * @retval #AZ_ERROR_DEPENDENCY_NOT_PROVIDED No platform implementation was supplied to support this
 * function.
 * @retval #AZ_ERROR_ARG Invalid argument.
 */
AZ_NODISCARD az_result az_platform_mutex_destroy(az_platform_mutex* mutex_handle);

AZ_NODISCARD az_platform_hash_table* az_platform_hash_table_create(size_t max_size);

AZ_NODISCARD az_result az_platform_hash_table_add(az_platform_hash_table* hash_table, az_span key, void* item, size_t item_size);

AZ_NODISCARD az_result az_platform_hash_table_remove(az_platform_hash_table* hash_table, az_span key);

AZ_NODISCARD void* az_platform_hash_table_get_by_key(az_platform_hash_table* hash_table, az_span key);

// AZ_NODISCARD az_result az_platform_hash_table_get_keys(az_platform_hash_table* hash_table, az_span** keys, size_t* out_size);

AZ_NODISCARD void* az_platform_hash_table_find(az_platform_hash_table* hash_table, bool predicate(az_span key, void* value, void* user_data), void* predicate_user_data);


#endif // __APPLE__

#include <azure/core/_az_cfg_suffix.h>

#endif // _az_PLATFORM_H

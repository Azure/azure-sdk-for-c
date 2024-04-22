// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

/**
 * @file
 *
 * @brief Api for supporting general retry logic.
 *
 * @note You MUST NOT use any symbols (macros, functions, structures, enums, etc.)
 * prefixed with an underscore ('_') directly in your application code. These symbols
 * are part of Azure SDK's internal implementation; we do not document these symbols
 * and they are subject to change in future versions of the SDK which would break your code.
 */

#ifndef _az_RETRY_H
#define _az_RETRY_H

#include <azure/core/az_result.h>
#include <stdbool.h>
#include <stdint.h>

#include <azure/core/_az_cfg_prefix.h>

/**
 * @brief Calculates the recommended delay before retrying an operation that failed.
 *
 * @param[in] operation_msec The time it took, in milliseconds, to perform the operation that
 *                           failed.
 * @param[in] attempt The number of failed retry attempts.
 * @param[in] min_retry_delay_msec The minimum time, in milliseconds, to wait before a retry.
 * @param[in] max_retry_delay_msec The maximum time, in milliseconds, to wait before a retry.
 * @param[in] random_jitter_msec A random value between 0 and the maximum allowed jitter, in
 * milliseconds.
 * @pre \p operation_msec must be between 0 and INT32_MAX - 1.
 * @pre \p attempt must be between 0 and INT16_MAX - 1.
 * @pre \p min_retry_delay_msec must be between 0 and INT32_MAX - 1.
 * @pre \p max_retry_delay_msec must be between 0 and INT32_MAX - 1.
 * @pre \p random_jitter_msec must be between 0 and INT32_MAX - 1.
 * @return The recommended delay in milliseconds.
 */
AZ_NODISCARD int32_t az_retry_calculate_delay(
    int32_t operation_msec,
    int16_t attempt,
    int32_t min_retry_delay_msec,
    int32_t max_retry_delay_msec,
    int32_t random_jitter_msec);

#include <azure/core/_az_cfg_suffix.h>

#endif // _az_RETRY_H

// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

/**
 * @file az_iot_common.h
 *
 * @brief Azure IoT common definitions.
 *
 * @note You MUST NOT use any symbols (macros, functions, structures, enums, etc.)
 * prefixed with an underscore ('_') directly in your application code. These symbols
 * are part of Azure SDK's internal implementation; we do not document these symbols
 * and they are subject to change in future versions of the SDK which would break your code.
 */

#ifndef _az_IOT_CORE_INTERNAL_H
#define _az_IOT_CORE_INTERNAL_H

#include <az_result.h>
#include <az_span.h>

#include <stdbool.h>
#include <stdint.h>

#include <_az_cfg_prefix.h>

/**
 * @brief Gives the length, in bytes, of the string that would represent the given number.
 *
 * @param[in] number The number whose length, as a string, is to be evaluated.
 * @return The length (not considering null terminator) of the string that would represent the given
 * number.
 */
AZ_NODISCARD int32_t _az_iot_u32toa_size(uint32_t number);

#include <_az_cfg_suffix.h>

#endif //!_az_IOT_CORE_INTERNAL_H

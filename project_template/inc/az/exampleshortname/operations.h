// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

/** @file operations.h */
#pragma once
#include <stdint.h>
#include <az/exampleshortname/export.h>
#ifdef __cplusplus
extern "C" {
#endif
/** @brief add two numbers together
 *  @param[in] a
 *  @param[in] b
 *  @return @p a + @p b
 */
AZ_EXAMPLESHORTNAME_EXPORT int32_t az_exampleshortname_add_two_numbers(int32_t a, int32_t b);

#ifdef __cplusplus
}
#endif
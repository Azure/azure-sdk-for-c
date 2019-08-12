// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

/** @file operations.h */
#ifndef AZ_EXAMPLESHORTNAME_OPERATIONS_H_
#define AZ_EXAMPLESHORTNAME_OPERATIONS_H_

#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
/** @brief do an example operation
 *  @retval b if @p a == 0
 *  @retval a otherwise
 */
int32_t az_exampleshortname_operation(int32_t a, int32_t b);

#ifdef __cplusplus
}
#endif

#endif

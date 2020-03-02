// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

/**
 * @file _az_cfg_prefix.h
 *
 * @brief adding extern C and pragma
 */

#ifdef __cplusplus
extern "C"
{
#endif // __cplusplus

#ifdef _MSC_VER
#pragma warning(push)
#elif defined(__GNUC__) // !_MSC_VER
#pragma GCC diagnostic push
#elif defined(__clang__) // !_MSC_VER !__clang__
#pragma clang diagnostic push
#endif // _MSC_VER

#include <_az_cfg.h>

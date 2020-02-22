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
#endif

#ifdef _MSC_VER
#pragma warning(push)
#elif defined(__GNUC__)
#pragma GCC diagnostic push
#elif defined(__clang__)
#pragma clang diagnostic push
#endif

#include <_az_cfg.h>

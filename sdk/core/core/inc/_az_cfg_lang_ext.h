// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#ifndef AZ_CFG_LANG_EXT_H
#define AZ_CFG_LANG_EXT_H

#ifdef _MSC_VER
#define AZ_INLINE __forceinline
#else
#define AZ_INLINE __attribute__((always_inline))
#endif

#endif

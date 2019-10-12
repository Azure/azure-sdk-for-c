// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#ifdef _MSC_VER
// warning C4204: nonstandard extension used: non-constant aggregate initializer
#pragma warning(disable : 4204)

// warning C4996: This function or variable may be unsafe. Consider using ..._s instead. To disable
// deprecation, use _CRT_SECURE_NO_WARNINGS. See online help for details.
#define _CRT_SECURE_NO_WARNINGS
#endif

#ifndef AZ_CFG_H
#define AZ_CFG_H

#ifdef _MSC_VER
#define AZ_INLINE static __forceinline
#elif defined(__GNUC__) || defined(__clang__)
#define AZ_INLINE static inline __attribute__((always_inline))
#else
#define AZ_INLINE static inline
#endif

#ifdef __GNUC__
#define AZ_FALLTHROUGH __attribute__((fallthrough))
#else
#define AZ_FALLTHROUGH \
  do { \
  } while (0)
#endif

#endif

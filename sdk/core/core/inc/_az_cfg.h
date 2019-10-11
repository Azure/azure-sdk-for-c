// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#ifdef _MSC_VER
// warning C4204: nonstandard extension used: non-constant aggregate initializer
#pragma warning(disable : 4204)
#endif

#ifndef AZ_CFG_H
#define AZ_CFG_H

#ifdef _MSC_VER
#define AZ_INLINE static __forceinline
#elif defined(__GNUC__)
#define AZ_INLINE static inline __attribute__((always_inline)) __extension__
#elif defined(__clang__)
#define AZ_INLINE static inline __attribute__((always_inline))
#else
#define AZ_INLINE static inline
#endif

#ifdef __GNUC__
#define AZ_FALLTHROUGH __attribute__((fallthrough)) __extension__
#else
#define AZ_FALLTHROUGH \
  do { \
  } while (0)
#endif

#endif

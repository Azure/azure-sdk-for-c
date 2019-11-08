// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#ifdef _MSC_VER

// warning C4204: nonstandard extension used: non-constant aggregate initializer
#pragma warning(disable : 4204)

// warning C4221: nonstandard extension used: '...': cannot be initialized using address of
// automatic variable '...'
#pragma warning(disable : 4221)

// warning C4996: This function or variable may be unsafe. Consider using ..._s instead.
#pragma warning(disable : 4996)

// warning C4820: '<unnamed-tag>': '4' bytes padding added after data member '...'
#pragma warning(disable : 4820)

// warning C5045: Compiler will insert Spectre mitigation for memory load if /Qspectre switch specified
#pragma warning(disable : 5045)

#elif defined(__GNUC__)

#pragma GCC diagnostic ignored "-Wmissing-braces"

#elif defined(__clang__)

#pragma clang diagnostic ignored "-Wmissing-field-initializers"
#pragma clang diagnostic ignored "-Wmissing-braces"

#endif

#ifndef AZ_CFG_H
#define AZ_CFG_H

#ifdef _MSC_VER
#define AZ_INLINE static __forceinline
#elif defined(__GNUC__) || defined(__clang__)
#define AZ_INLINE \
  __attribute__((always_inline)) \
  static inline
#else
#define AZ_INLINE static inline
#endif

#if defined(__GNUC__) && __GNUC__ >= 7
#define AZ_FALLTHROUGH __attribute__((fallthrough))
#else
#define AZ_FALLTHROUGH \
  do { \
  } while (0)
#endif

#ifdef _MSC_VER
#define AZ_NODISCARD _Check_return_
#elif defined(__GNUC__) || defined(__clang__)
#define AZ_NODISCARD __attribute__((warn_unused_result))
#else
#define AZ_NODISCARD
#endif

#endif

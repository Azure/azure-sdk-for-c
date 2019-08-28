// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#ifndef AZ_DIGIT_H
#define AZ_DIGIT_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

inline bool az_in_range(char const min, char const max, char const c) {
  return min <= c && c <= max;
}

inline bool az_is_digit(char const c) {
  return az_in_range('0', '9', c);
}

inline uint8_t az_to_digit(char const c) {
  return c - '0';
}

#ifdef __cplusplus
}
#endif

#endif

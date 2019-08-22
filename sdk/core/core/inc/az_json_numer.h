// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#ifndef AZ_JSON_STATE_H
#define AZ_JSON_STATE_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

enum {
  AZ_JSON_NUMBER_STATE_ZERO  = 0,
  AZ_JSON_NUMBER_STATE_DIGIT = '1',
  AZ_JSON_NUMBER_STATE_MINUS = '-',
};

typedef char az_json_number_state_tag;

typedef struct {
  az_json_number_state_tag tag;
  union {
    uint64_t digit;
  };
} az_json_number_state;

#ifdef __cplusplus
}
#endif

#endif

// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#ifndef AZ_JSON_NUMBER_STATE_H
#define AZ_JSON_NUMBER_STATE_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
  // '-': 0 => INT0, 1..9 => INT
  AZ_JSON_NUMBER_STATE_SIGN,
  // '-0': . => FRACTION
  AZ_JSON_NUMBER_STATE_INT0,
  // '-123': 0..9 => INT, . => FRACTION
  AZ_JSON_NUMBER_STATE_INT,
  // '-1234.56: 0..9 => FRACTION, e|E => E
  AZ_JSON_NUMBER_STATE_FRACTION,
  // '-1234.56e': -|+|0 => E_SIGN, 1..9 => E_INT
  AZ_JSON_NUMBER_STATE_E,
  // '-1234.56e-00': 0 => E_SIGN, 1..9 => E_INT
  AZ_JSON_NUMBER_STATE_E_SIGN,
  // '-1234.56e-0059': 1..9 => E_INT
  AZ_JSON_NUMBER_STATE_E_INT
} az_json_number_state_tag;

typedef struct {
  // a signed value.
  double number;
  // a multiplier for a fraction digit
  double multiplier;
} az_json_number_state_fraction;

typedef struct {
  // a signed value.
  double value;
  // a signed integer value.
  int e_sign;
} az_json_number_state_e_sign;

typedef struct {
  // a signed value.
  double fraction;
  // a signed integer value.
  int e_int;
} az_json_number_state_e_int;

typedef struct {
  az_json_number_state_tag tag;
  union {
    // -1,1
    int sign;
    // -1,1
    int int0;
    // a signed integer value.
    double int_;
    az_json_number_state_fraction fraction;
    // a signed value.
    double e;
    az_json_number_state_e_sign e_sign;
    az_json_number_state_e_int e_int;
  };
} az_json_number_state;

inline az_json_number_state az_json_number_state_sign(int sign) {
  return (az_json_number_state){ .tag = AZ_JSON_NUMBER_STATE_SIGN, .sign = sign };
}

inline az_json_number_state az_json_number_state_int0(int int0) {
  return (az_json_number_state){ .tag = AZ_JSON_NUMBER_STATE_INT0, .int0 = int0 };
}

#ifdef __cplusplus
}
#endif

#endif

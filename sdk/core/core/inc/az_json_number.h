// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#ifndef AZ_JSON_NUMBER_H
#define AZ_JSON_NUMBER_H

#include <stdint.h>
#include <az_static_assert.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef int8_t az_sign;

enum {
  AZ_JSON_TERMINAL = '\xFF',
};

typedef enum {
  AZ_JSON_NUMBER_MINUS,
  AZ_JSON_NUMBER_ZERO,
  AZ_JSON_NUMBER_INTEGER,
  AZ_JSON_NUMBER_DOT,
  AZ_JSON_NUMBER_FRACTION,
  AZ_JSON_NUMBER_E,
  AZ_JSON_NUMBER_E_SIGN,
  AZ_JSON_NUMBER_E_NUMBER,
  AZ_JSON_NUMBER_DONE,
  AZ_JSON_NUMBER_ERROR,
} az_json_number_tag;

typedef az_sign az_json_number_zero;

typedef struct {
  double number;
  double multiplier;
} az_json_number_multiplier;

typedef az_json_number_multiplier az_json_number_integer;

typedef az_json_number_integer az_json_number_dot;

typedef az_json_number_multiplier az_json_number_fraction;

typedef double az_json_number_e;

typedef struct {
  double number;
  az_sign e_sign;
} az_json_number_e_sign;

typedef struct {
  double number;
  int16_t e_number;
  az_sign e_sign;
} az_json_number_e_number;

typedef struct {
  double number;
  char next;
} az_json_number_done;

typedef struct {
  az_json_number_tag tag;
  union {
    // multiplier
    az_json_number_zero zero;
    az_json_number_integer integer;
    az_json_number_integer dot;
    az_json_number_fraction fraction;
    az_json_number_e e;
    az_json_number_e_sign e_sign;
    az_json_number_e_number e_number;
    az_json_number_done done;
  };
} az_json_number;

inline az_json_number az_json_number_create_minus() {
  return (az_json_number){ .tag = AZ_JSON_NUMBER_MINUS };
}

inline az_json_number az_json_number_create_zero(az_sign const sign) {
  return (az_json_number){ .tag = AZ_JSON_NUMBER_ZERO, .zero = sign };
}

inline az_json_number az_json_number_create_integer(az_json_number_integer const integer) {
  return (az_json_number){ .tag = AZ_JSON_NUMBER_INTEGER, .integer = integer };
}

inline az_json_number az_json_number_create_dot(az_json_number_dot const dot) {
  return (az_json_number){ .tag = AZ_JSON_NUMBER_DOT, .dot = dot };
}

inline az_json_number az_json_number_create_fraction(az_json_number_fraction const fraction) {
  return (az_json_number){ .tag = AZ_JSON_NUMBER_FRACTION, .fraction = fraction };
}

inline az_json_number az_json_number_create_e(az_json_number_e e) {
  return (az_json_number){ .tag = AZ_JSON_NUMBER_E, .e = e };
}

inline az_json_number az_json_number_create_e_sign(az_json_number_e_sign const e_sign) {
  return (az_json_number){ .tag = AZ_JSON_NUMBER_E_SIGN, .e_sign = e_sign };
}

inline az_json_number az_json_number_create_e_number(az_json_number_e_number const e_number) {
  return (az_json_number){ .tag = AZ_JSON_NUMBER_E_NUMBER, .e_number = e_number };
}

inline az_json_number az_json_number_create_done(az_json_number_done const done) {
  return (az_json_number){ .tag = AZ_JSON_NUMBER_DONE, .done = done };
}

inline az_json_number az_json_number_create_error() {
  return (az_json_number){ .tag = AZ_JSON_NUMBER_ERROR };
}

az_json_number az_json_number_try_parse(char const c);

az_json_number az_json_number_parse(az_json_number const state, char const c);

#ifdef __cplusplus
}
#endif

#endif

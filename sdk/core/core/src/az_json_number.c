// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <az_json_number.h>

#include <az_digit.h>
#include <stdbool.h>
#include <math.h>

inline bool az_is_e(char const c) {
  return c == 'e' || c == 'E';
}

az_json_number az_json_number_integer_add(az_json_number_integer const integer, char c) {
  double const multiplier = integer.multiplier;
  return az_json_number_create_integer((az_json_number_integer){
    .number = integer.number * 10 + multiplier * az_to_digit(c),
    .multiplier = multiplier,
  });
}

az_json_number az_json_number_sign_parse(az_sign sign, char const c, az_json_number const default_) {
  if (c == '0') {
    return az_json_number_create_zero(sign);
  }
  if (az_is_digit(c)) {
    return az_json_number_integer_add(
      (az_json_number_integer){ .number = 0, .multiplier = sign },
      c
    );
  }
  return default_;
}

az_json_number az_json_number_none_parse(char const c) {
  if (c == '-') {
    return az_json_number_create_minus();
  }
  return az_json_number_sign_parse(1, c, az_json_number_create_none());
}

az_json_number az_json_number_minus_parse(char const c) {
  return az_json_number_sign_parse(-1, c, az_json_number_create_error());
}

az_json_number az_json_number_fraction_parse_end(double const number, char const c) {
  if (az_is_e(c)) {
    return az_json_number_create_e(number);
  }
  return az_json_number_create_done((az_json_number_done){ .number = number, .next = c });
}

az_json_number az_json_number_integer_parse_end(az_json_number_integer const integer, char const c) {
  if (c == '.') {
    return az_json_number_create_dot(integer);
  }
  return az_json_number_fraction_parse_end(integer.number, c);
}

az_json_number az_json_number_zero_parse(az_sign const sign, char const c) {
  return az_json_number_integer_parse_end((az_json_number_integer){ .number = 0, .multiplier = sign }, c);
}

az_json_number az_json_number_integer_parse(az_json_number_integer const integer, char const c) {
  if (az_is_digit(c)) {
    return az_json_number_integer_add(integer, c);
  }
  return az_json_number_integer_parse_end(integer, c);
}

az_json_number az_json_number_fraction_add(az_json_number_fraction fraction, char const c) {
  double const multiplier = fraction.multiplier * 0.1;
  return az_json_number_create_fraction((az_json_number_fraction){
    .number = fraction.number + multiplier * az_to_digit(c),
    .multiplier = multiplier,
  });
}

az_json_number az_json_number_dot_parse(az_json_number_integer const integer, char const c) {
  if (az_is_digit(c)) {
    return az_json_number_fraction_add(integer, c);
  }
  return az_json_number_create_error();
}

az_json_number az_json_number_fraction_parse(az_json_number_fraction const fraction, char const c) {
  if (az_is_digit(c)) {
    return az_json_number_fraction_add(fraction, c);
  }
  return az_json_number_fraction_parse_end(fraction.number, c);
}

az_json_number az_json_number_e_sign_parse(az_json_number_e_sign const e_sign, char const c) {
  if (az_is_digit(c)) {
    az_sign const sign = e_sign.e_sign;
    return az_json_number_create_e_number((az_json_number_e_number){
      .number = e_sign.number,
      .e_sign = sign,
      .e_number = sign * az_to_digit(c),
    });
  }
  return az_json_number_create_error();
}

az_json_number az_json_number_e_parse(az_json_number_e const e, char const c) {
  switch (c) {
    case '+':
      return az_json_number_create_e_sign((az_json_number_e_sign){ .number = e, .e_sign = 1 });
    case '-':
      return az_json_number_create_e_sign((az_json_number_e_sign){ .number = e, .e_sign = -1 });
  }
  return az_json_number_e_sign_parse((az_json_number_e_sign){ .number = e, .e_sign = 1 }, c);
}

az_json_number az_json_number_e_number_parse(az_json_number_e_number const e_number, char const c) {
  if (az_is_digit(c)) {
    const sign = e_number.e_sign;
    return az_json_number_create_e_number((az_json_number_e_number){
      .number = e_number.number,
      .e_sign = sign,
      .e_number = e_number.e_number * 10 + sign * az_to_digit(c),
    });
  }
  return az_json_number_create_done((az_json_number_done){
    .number = e_number.number * pow(10.0, e_number.e_number),
    .next = c,
  });
}

az_json_number az_json_number_parse(az_json_number const state, char const c) {
  switch (state.tag) {
    case AZ_JSON_NUMBER_NONE:
      return az_json_number_none_parse(c);
    case AZ_JSON_NUMBER_MINUS:
      return az_json_number_minus_parse(c);
    case AZ_JSON_NUMBER_ZERO:
      return az_json_number_zero_parse(state.zero, c);
    case AZ_JSON_NUMBER_INTEGER:
      return az_json_number_integer_parse(state.integer, c);
    case AZ_JSON_NUMBER_DOT:
      return az_json_number_dot_parse(state.dot, c);
    case AZ_JSON_NUMBER_FRACTION:
      return az_json_number_fraction_parse(state.fraction, c);
    case AZ_JSON_NUMBER_E:
      return az_json_number_e_parse(state.e, c);
    case AZ_JSON_NUMBER_E_SIGN:
      return az_json_number_e_sign_parse(state.e_sign, c);
    case AZ_JSON_NUMBER_E_NUMBER:
      return az_json_number_e_number_parse(state.e_number, c);
  }
  return az_json_number_create_error();
}

// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <az_json_string.h>

#include <az_digit.h>
#include <az_cstr.h>

az_json_string az_json_string_none_parse(char const c) {
  if (c == '"') {
    return az_json_string_create_open();
  }
  return az_json_string_create_none();
}

az_json_string az_json_string_char_parse(int32_t const position, char const c) {
  switch (c) {
    case '\\':
      return az_json_string_create_esc(position);
    case '"':
      return az_json_string_create_close(position);
  }
  if (' ' <= c) {
    return az_json_string_create_char((az_json_string_char){ .position = position, .code = c });
  }
  return az_json_string_create_error();
}

char az_json_string_esc_decode(char c) {
  switch (c) {
    case '"':
    case '\\':
    case '/':
      return c;
    case 'b':
      return '\b';
    case 'f':
      return '\f';
    case 'n':
      return '\n';
    case 'r':
      return '\r';
    case 't':
      return '\t';
  }
  return 0;
}

az_json_string az_json_string_esc_parse(int32_t position, char const c) {
  if (c == 'u') {
    return az_json_string_create_u((az_json_string_u){ .position = position, .code = 0, .i = 0 });
  }
  char const e = az_json_string_esc_decode(c);
  if (e != 0) {
    return az_json_string_create_char((az_json_string_char){ .position = position, .code = c });
  }
  return az_json_string_create_error();
}

int8_t az_to_hex_digit(char const c) {
  if (az_is_digit(c)) {
    return az_to_digit(c);
  }
  if (az_in_range('a', 'f', c)) {
    return c - ('a' - 10);
  }
  if (az_in_range('A', 'F', c)) {
    return c - ('A' - 10);
  }
  return -1;
}

az_json_string az_json_string_u_parse(int32_t position, int32_t const old_code, int8_t const old_i, char const c) {
  int8_t const i = old_i + 1;
  int8_t const hex = az_to_hex_digit(c);
  if (hex < 0) {
    return az_json_string_create_error();
  }
  int32_t const code = (old_code << 4) + hex;
  if (i == 4) {
    return az_json_string_create_char((az_json_string_char){ .position = position, .code = code });
  }
  return az_json_string_create_u((az_json_string_u){ .position = position, .code = code, .i = i });
}

inline az_json_string az_json_string_close_parse(int32_t position, char c) {
  return az_json_string_create_done((az_json_string_done){ .position = position, .next = c });
}

az_json_string az_json_string_parse(az_json_string const state, char const c) {
  switch (state.tag) {
    case AZ_JSON_STRING_NONE:
      return az_json_string_none_parse(c);
    case AZ_JSON_STRING_OPEN:
      return az_json_string_char_parse(0, c);
    case AZ_JSON_STRING_CHAR:
      return az_json_string_char_parse(state.char_.position + 1, c);
    case AZ_JSON_STRING_ESC:
      return az_json_string_esc_parse(state.esc + 1, c);
    case AZ_JSON_STRING_U:
      az_json_string_u const u = state.u;
      return az_json_string_u_parse(u.position + 1, u.code, u.i, c);
    case AZ_JSON_STRING_CLOSE:
      return az_json_string_close_parse(state.close, c);
  }
  return az_json_string_create_error();
}
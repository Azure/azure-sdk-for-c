// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <az_json_symbol.h>

inline az_json_symbol az_json_symbol_none_parse(char const c) {
  switch (c) {
    case '{':
    case '}':
    case '[':
    case ']':
    case ',':
    case ':':
      return az_json_symbol_create_char(c);
  }
  return az_json_symbol_create_none();
}

inline az_json_symbol az_json_symbol_char_parse(char const c, char const next) {
  return az_json_symbol_create_done((az_json_symbol_done){ .char_ = c, .next = next });
}

az_json_symbol az_json_symbol_parse(az_json_symbol const state, char const c) {
  switch (state.tag) {
    case AZ_JSON_SYMBOL_NONE:
      return az_json_symbol_none_parse(c);
    case AZ_JSON_SYMBOL_CHAR:
      return az_json_symbol_char_parse(state.char_, c);
  }
  return az_json_symbol_create_error();
}